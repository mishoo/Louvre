#include <protocols/ForeignToplevelManagement/GForeignToplevelManager.h>
#include <protocols/SinglePixelBuffer/GSinglePixelBufferManager.h>
#include <protocols/ForeignToplevelList/GForeignToplevelList.h>
#include <protocols/RelativePointer/GRelativePointerManager.h>
#include <protocols/FractionalScale/GFractionalScaleManager.h>
#include <protocols/PointerConstraints/GPointerConstraints.h>
#include <protocols/TearingControl/GTearingControlManager.h>
#include <protocols/XdgDecoration/GXdgDecorationManager.h>
#include <protocols/GammaControl/GGammaControlManager.h>
#include <protocols/PointerGestures/GPointerGestures.h>
#include <protocols/SessionLock/GSessionLockManager.h>
#include <protocols/ContentType/GContentTypeManager.h>
#include <protocols/IdleInhibit/GIdleInhibitManager.h>
#include <protocols/PresentationTime/GPresentation.h>
#include <protocols/ScreenCopy/GScreenCopyManager.h>
#include <protocols/XdgOutput/GXdgOutputManager.h>
#include <protocols/Wayland/GDataDeviceManager.h>
#include <protocols/LinuxDMABuf/GLinuxDMABuf.h>
#include <protocols/IdleNotify/GIdleNotifier.h>
#include <protocols/Viewporter/GViewporter.h>
#include <protocols/LayerShell/GLayerShell.h>
#include <protocols/Wayland/GSubcompositor.h>
#include <protocols/XdgShell/GXdgWmBase.h>
#include <protocols/Wayland/GCompositor.h>
#include <protocols/Wayland/GSeat.h>
#include <LSessionLockManager.h>
#include <LSessionLockRole.h>
#include <LCompositor.h>
#include <LToplevelRole.h>
#include <LCursor.h>
#include <LSubsurfaceRole.h>
#include <LPointer.h>
#include <LKeyboard.h>
#include <LTouch.h>
#include <LSurface.h>
#include <LDND.h>
#include <LClipboard.h>
#include <LOutput.h>
#include <LSeat.h>
#include <LPopupRole.h>
#include <LCursorRole.h>
#include <LLog.h>
#include <LXCursor.h>
#include <LClient.h>
#include <LDNDIconRole.h>
#include <LGlobal.h>

using namespace Louvre;
using namespace Louvre::Protocols;

//! [createGlobalsRequest]
bool LCompositor::createGlobalsRequest()
{
    // Allows clients to create surfaces and regions
    createGlobal<Wayland::GCompositor>();

    // Allows clients to receive pointer, keyboard, and touch events
    createGlobal<Wayland::GSeat>();

    // Provides detailed information of pointer movement
    createGlobal<RelativePointer::GRelativePointerManager>();

    // Allows clients to request setting pointer constraints
    createGlobal<PointerConstraints::GPointerConstraints>();

    // Allows clients to receive swipe, pinch, and hold pointer gestures
    createGlobal<PointerGestures::GPointerGestures>();

    // Enables drag & drop and clipboard data sharing between clients
    createGlobal<Wayland::GDataDeviceManager>();

    // Allows clients to create subsurface roles
    createGlobal<Wayland::GSubcompositor>();

    // Allows clients to create toplevel and popup roles
    createGlobal<XdgShell::GXdgWmBase>();

    // Allows clients to request modifying the state of foreign toplevels
    createGlobal<ForeignToplevelManagement::GForeignToplevelManager>();

    // Allows clients to get handles of foreign toplevels
    createGlobal<ForeignToplevelList::GForeignToplevelList>();

    // Provides additional info about outputs
    createGlobal<XdgOutput::GXdgOutputManager>();

    // Allow negotiation of server-side or client-side decorations
    createGlobal<XdgDecoration::GXdgDecorationManager>();

    // Allow clients to adjust their surfaces buffers to fractional scales
    createGlobal<FractionalScale::GFractionalScaleManager>();

    // Allow clients to request setting the gamma LUT of outputs
    createGlobal<GammaControl::GGammaControlManager>();

    // Allow clients to create DMA buffers
    if (!LTexture::supportedDMAFormats().empty())
        createGlobal<LinuxDMABuf::GLinuxDMABuf>();

    // Provides detailed information of how surfaces are presented
    createGlobal<PresentationTime::GPresentation>();

    // Allows clients to request locking the user session with arbitrary graphics
    createGlobal<SessionLock::GSessionLockManager>();

    // Allows clients to notify their preference of vsync for specific surfaces
    createGlobal<TearingControl::GTearingControlManager>();

    // Allows clients to clip and scale buffers
    createGlobal<Viewporter::GViewporter>();

    // Allows clients to capture outputs
    createGlobal<ScreenCopy::GScreenCopyManager>();

    // Allows clients to create wlr_layer_shell surfaces
    createGlobal<LayerShell::GLayerShell>();

    // Allows clients to create single pixel buffers (requires Viewporter::GViewporter)
    createGlobal<SinglePixelBuffer::GSinglePixelBufferManager>();

    // Allows clients to provide a hint about the content type being displayed by surfaces
    createGlobal<ContentType::GContentTypeManager>();

    // Notifies clients if the user has been idle for a given amount of time
    createGlobal<IdleNotify::GIdleNotifier>();

    // Allows clients to request inhibition of the compositor's idle state
    createGlobal<IdleInhibit::GIdleInhibitManager>();

    return true;
}
//! [createGlobalsRequest]

//! [globalsFilter]
bool LCompositor::globalsFilter(LClient *client, LGlobal *global)
{
    L_UNUSED(client)
    L_UNUSED(global)
    return true;
}
//! [globalsFilter]

//! [initialized]
void LCompositor::initialized()
{
    Int32 totalWidth = 0;

    // Initializes and arranges outputs from left to right
    for (LOutput *output : seat()->outputs())
    {
        // Sets a scale factor of 2 when DPI >= 200
        output->setScale(output->dpi() >= 200 ? 2.f : 1.f);

        // Change it if any of your displays is rotated/flipped
        output->setTransform(LTransform::Normal);

        output->setPos(LPoint(totalWidth, 0));
        totalWidth += output->size().w();
        addOutput(output);
        output->repaint();
    }
}
//! [initialized]

//! [uninitialized]
void LCompositor::uninitialized()
{
    /* No default implementation */
}
//! [uninitialized]

//! [createObjectRequest]
LFactoryObject *LCompositor::createObjectRequest(LFactoryObject::Type objectType, const void *params)
{
    L_UNUSED(objectType)
    L_UNUSED(params)

    /* If nullptr is returned, Louvre creates an instance of the base class */
    return nullptr;
}
//! [createObjectRequest]


//! [onAnticipatedObjectDestruction]
void LCompositor::onAnticipatedObjectDestruction(LFactoryObject *object)
{
    L_UNUSED(object)
}
//! [onAnticipatedObjectDestruction]
