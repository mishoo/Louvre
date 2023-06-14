#include <private/LCompositorPrivate.h>
#include <private/LClientPrivate.h>
#include <private/LSeatPrivate.h>
#include <private/LSurfacePrivate.h>
#include <private/LOutputPrivate.h>
#include <private/LCursorPrivate.h>

#include <protocols/Wayland/private/GOutputPrivate.h>

#include <LNamespaces.h>
#include <LPopupRole.h>

#include <stdio.h>
#include <sys/poll.h>
#include <thread>
#include <unistd.h>
#include <LToplevelRole.h>
#include <LCursor.h>
#include <LSubsurfaceRole.h>
#include <LPointer.h>
#include <LKeyboard.h>
#include <LDNDManager.h>
#include <dlfcn.h>
#include <LLog.h>

using namespace Louvre::Protocols::Wayland;

static LCompositor *s_compositor = nullptr;

LCompositor::LCompositor()
{
    LLog::init();
    LSurface::LSurfacePrivate::getEGLFunctions();
    m_imp = new LCompositorPrivate();
    imp()->compositor = this;
}

LCompositor *LCompositor::compositor()
{
    return s_compositor;
}

bool LCompositor::isGraphicBackendInitialized() const
{
    return imp()->isGraphicBackendInitialized;
}

bool LCompositor::isInputBackendInitialized() const
{
    return imp()->isInputBackendInitialized;
}

bool LCompositor::loadGraphicBackend(const char *path)
{
    return imp()->loadGraphicBackend(path);
}

bool LCompositor::loadInputBackend(const char *path)
{
    return imp()->loadInputBackend(path);
}

Int32 LCompositor::globalScale() const
{
    return imp()->globalScale;
}

LCompositor::CompositorState LCompositor::state() const
{
    return imp()->state;
}

LCompositor::~LCompositor()
{
    delete m_imp;
}

bool LCompositor::start()
{
    if (compositor())
    {
        LLog::warning("[compositor] Compositor already running. Two Louvre compositors can not live in the same process.");
        return false;
    }

    s_compositor = this;

    if (state() != CompositorState::Uninitialized)
    {
        LLog::warning("[compositor] Attempting to start a compositor already running. Ignoring...");
        return false;
    }

    imp()->threadId = std::this_thread::get_id();
    imp()->state = CompositorState::Initializing;

    if (!imp()->initWayland())
    {
        LLog::fatal("[compositor] Failed to init Wayland.");
        goto fail;
    }

    if (!imp()->initSeat())
    {
        LLog::fatal("[compositor] Failed to init seat.");
        goto fail;
    }

    if (!imp()->initGraphicBackend())
    {
        LLog::fatal("[compositor] Failed to init Graphic backend.");
        goto fail;
    }

    if (!imp()->inputBackend)
    {
        LLog::warning("[compositor] User did not load an input backend. Trying the Libinput backend...");

        if (!loadInputBackend("/usr/etc/Louvre/backends/libLInputBackendLibinput.so"))
        {
            LLog::fatal("[compositor] No input backend found. Stopping compositor...");
            goto fail;
        }
    }

    if (!imp()->inputBackend->initialize(seat()))
    {
        LLog::fatal("[compositor] Failed to initialize input backend. Stopping compositor...");
        goto fail;
    }

    LLog::debug("[compositor] Input backend initialized successfully.");
    imp()->isInputBackendInitialized = true;

    seat()->initialized();

    imp()->state = CompositorState::Initialized;
    initialized();

    imp()->fdSet.events = POLLIN | POLLOUT | POLLHUP;
    imp()->fdSet.revents = 0;

    return true;

    fail:
    imp()->uinitCompositor();
    return false;
}

Int32 LCompositor::processLoop(Int32 msTimeout)
{
    poll(&imp()->fdSet, 1, msTimeout);
    imp()->renderMutex.lock();
    imp()->processRemovedGlobals();

    // DND
    if (seat()->dndManager()->imp()->destDidNotRequestReceive >= 3)
        seat()->dndManager()->cancel();

    if (seat()->dndManager()->imp()->dropped && seat()->dndManager()->imp()->destDidNotRequestReceive < 3)
        seat()->dndManager()->imp()->destDidNotRequestReceive++;

    wl_event_loop_dispatch(imp()->eventLoop, 0);
    flushClients();
    cursor()->imp()->textureUpdate();
    imp()->renderMutex.unlock();
    return 1;
}

void LCompositor::finish()
{
    abort();

    /* TODO:
    if (imp()->started)
    {
        if (imp()->isInputBackendInitialized)
            imp()->inputBackend->uninitialize(seat());

        if (imp()->isGraphicBackendInitialized)
            imp()->graphicBackend->uninitialize(this);
    }
    */
}

void LCompositor::LCompositorPrivate::raiseChildren(LSurface *surface)
{            
    surfaces.splice( surfaces.end(), surfaces, surface->imp()->compositorLink);

    surface->raised();
    surface->orderChanged();

    // Rise its children
    for (LSurface *children : surface->children())
        raiseChildren(children);
}

void LCompositor::raiseSurface(LSurface *surface)
{
    if (surface->subsurface())
    {
        raiseSurface(surface->parent());
        return;
    }

    imp()->raiseChildren(surface);
}

wl_display *LCompositor::display()
{
    return LCompositor::compositor()->imp()->display;
}

wl_event_source *LCompositor::addFdListener(int fd, void *userData, int (*callback)(int, unsigned int, void *), UInt32 flags)
{
    return wl_event_loop_add_fd(LCompositor::compositor()->imp()->eventLoop, fd, flags, callback, userData);
}

void LCompositor::removeFdListener(wl_event_source *source)
{
    wl_event_source_remove(source);
}

LCursor *LCompositor::cursor() const
{
    return imp()->cursor;
}

LSeat *LCompositor::seat() const
{
    return imp()->seat;
}

void LCompositor::repaintAllOutputs()
{
    for (list<LOutput*>::iterator it = imp()->outputs.begin(); it != imp()->outputs.end(); ++it)
        (*it)->repaint();
}

bool LCompositor::addOutput(LOutput *output)
{
    // Verifica que no se haya añadido previamente
    for (LOutput *o : outputs())
        if (o == output)
            return true;

    imp()->outputs.push_back(output);

    if (!output->imp()->initialize())
    {
        LLog::error("[Compositor] Failed to initialize output %s.", output->name());
        removeOutput(output);
        return false;
    }

    return true;
}

void LCompositor::removeOutput(LOutput *output)
{
    // Loop to check if output was added (initialized)
    for (list<LOutput*>::iterator it = imp()->outputs.begin(); it != imp()->outputs.end(); it++)
    {
        // Was initialized
        if (*it == output)
        {
            output->imp()->state = LOutput::PendingUninitialize;
            imp()->graphicBackend->uninitializeOutput(output);
            output->imp()->state = LOutput::Uninitialized;

            for (LSurface *s : surfaces())
                s->sendOutputLeaveEvent(output);

            imp()->outputs.remove(output);

            // Remove all wl_outputs from clients
            for (LClient *c : clients())
            {
                for (GOutput *g : c->outputGlobals())
                {
                    if (output == g->output())
                    {
                        g->client()->imp()->outputGlobals.erase(g->imp()->clientLink);
                        g->imp()->lOutput = nullptr;

                        // Break because clients can bind to to a wl_output global just once
                        break;
                    }
                }
            }

            // Safely remove global
            imp()->removeGlobal(output->imp()->global);

            // Find the new greatest output scale
            imp()->updateGlobalScale();

            cursor()->imp()->intersectedOutputs.remove(output);

            if (cursor()->imp()->output == output)
                cursor()->imp()->output = nullptr;

            return;
        }
    }
}

const list<LSurface *> &LCompositor::surfaces() const
{
    return imp()->surfaces;
}

const list<LOutput *> &LCompositor::outputs() const
{
    return imp()->outputs;
}

const list<LClient *> &LCompositor::clients() const
{
    return imp()->clients;
}

UInt32 LCompositor::nextSerial()
{
    return wl_display_next_serial(LCompositor::compositor()->display());
}

EGLDisplay LCompositor::eglDisplay()
{
    return LCompositor::compositor()->imp()->mainEGLDisplay;
}

EGLContext LCompositor::eglContext()
{
    return LCompositor::compositor()->imp()->mainEGLContext;
}

void LCompositor::flushClients()
{
    wl_display_flush_clients(LCompositor::display());
}

LClient *LCompositor::getClientFromNativeResource(wl_client *client)
{
    for (LClient *c : clients())
        if (c->client() == client)
            return c;
    return nullptr;
}

thread::id LCompositor::mainThreadId() const
{
    return imp()->threadId;
}
