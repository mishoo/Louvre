#include <protocols/ForeignToplevelManagement/GForeignToplevelManager.h>
#include <LSurface.h>
#include <LCursor.h>
#include <LCompositor.h>
#include <private/LCompositorPrivate.h>
#include <LScene.h>
#include <unistd.h>
#include <LLauncher.h>

#include "Keyboard.h"
#include "Global.h"
#include "LLayerRole.h"
#include "Output.h"
#include "Topbar.h"
#include "Surface.h"
#include "Toplevel.h"
#include "App.h"
#include "Client.h"
#include "Workspace.h"

Keyboard::Keyboard(const void *params) : LKeyboard(params) {}

void Keyboard::keyEvent(const LKeyboardKeyEvent &event)
{
    Output *output  { (Output*)cursor()->output()     };

    bool LEFT_META  { isKeyCodePressed(KEY_LEFTMETA)  };
    bool LEFT_SHIFT { isKeyCodePressed(KEY_LEFTSHIFT) };
    bool LEFT_ALT   { isKeyCodePressed(KEY_LEFTALT)   };
    bool LEFT_CTRL  { isKeyCodePressed(KEY_LEFTCTRL)  };

    /**** Initialize/Uninitialize all outputs ****/

    if (LEFT_SHIFT && LEFT_META && event.keyCode() == KEY_O && event.state() == LKeyboardKeyEvent::Pressed)
    {
        static bool initOutputs { false };

        if (initOutputs)
        {
            for (LOutput *o : seat()->outputs())
                compositor()->removeOutput(o);
        }
        else
        {
            for (LOutput *o : seat()->outputs())
                compositor()->addOutput(o);
        }

        initOutputs = !initOutputs;
    }

    if (output && event.state() == LKeyboardKeyEvent::Pressed)
    {
        /* Switch workspace */

        if (LEFT_ALT && LEFT_CTRL && output->currentWorkspace)
        {
            if (event.keyCode() == KEY_RIGHT && std::next(output->currentWorkspace->outputLink) != output->workspaces.end())
            {
                if (!output->animatedFullscreenToplevel)
                    output->setWorkspace(*std::next(output->currentWorkspace->outputLink), 450.f, 2.3f, 0.2f);
                return;
            }
            else if (event.keyCode() == KEY_LEFT && output->currentWorkspace != output->workspaces.front())
            {
                if (!output->animatedFullscreenToplevel)
                    output->setWorkspace(*std::prev(output->currentWorkspace->outputLink), 450.f, 2.3f, 0.2f);
                return;
            }
        }

        if (LEFT_META && LEFT_SHIFT)
        {
            /*********** Change focused layer role exclusive zone pref *********/

            if (isKeyCodePressed(KEY_L))
            {
                if (event.keyCode() == KEY_DOWN && seat()->pointer()->focus() && seat()->pointer()->focus()->layerRole())
                {
                    auto *layerRole { seat()->pointer()->focus()->layerRole() };
                    if (layerRole->exclusiveZone().prevZone())
                        layerRole->exclusiveZone().insertAfter(layerRole->exclusiveZone().prevZone()->prevZone());
                    return;
                }

                if (event.keyCode() == KEY_UP && seat()->pointer()->focus() && seat()->pointer()->focus()->layerRole())
                {
                    auto *layerRole { seat()->pointer()->focus()->layerRole() };
                    if (layerRole->exclusiveZone().nextZone())
                        layerRole->exclusiveZone().insertAfter(layerRole->exclusiveZone().nextZone());
                    return;
                }
            }

            /*********** Launch Wofi *********/

            if (isKeyCodePressed(KEY_W) && G::compositor()->wofi)
            {
                G::compositor()->wofi->dockIconClicked();
                return;
            }

            switch (event.keyCode())
            {

            /*********** Turn ON / OFF V-Sync *********/

            case KEY_V:
                output->enableVSync(!output->vSyncEnabled());
                output->topbar.update();
                break;

            /************ Change output mode ***********/

            case KEY_M:
                if (output->currentMode() == output->modes().back())
                    output->setMode(output->modes().front());
                else
                    output->setMode(
                        *(++std::find(
                            output->modes().begin(),
                            output->modes().end(),
                            output->currentMode()))
                        );
                break;

            /********** Change output transform **********/

            case KEY_T:
                if (output->transform() == LTransform::Flipped270)
                    output->setTransform(LTransform::Normal);
                else
                    output->setTransform((LTransform)((Int32)output->transform() + 1));
                break;

            /**** Increase fractional scaling by 0.25 ****/

            case KEY_UP:
                if (output->fractionalScale() < 3.f)
                {
                    output->setScale(output->fractionalScale() + 0.25);
                    output->repaint();
                }
                break;

            /**** Decrease fractional scaling by 0.25 ****/

            case KEY_DOWN:
                if (output->fractionalScale() > 0.25f)
                {
                    output->setScale(output->fractionalScale() - 0.25);
                    output->repaint();
                }
                break;

            /**** Destroy all current foreign toplevel managers ****/

            /* This only prevents current managers from receiving information about newly
             * created toplevel windows */
            case KEY_F:
                for (LClient *client : compositor()->clients())
                    while (!client->foreignToplevelManagerGlobals().empty())
                        client->foreignToplevelManagerGlobals().back()->finished();
                break;

            case KEY_PAGEDOWN:
            {
                Surface *s = (Surface*)focus();
                if (s && s->tl()) s->tl()->setMinimizedRequest();
                return;
            }

            case KEY_PAGEUP:
            {
                Surface *s = (Surface*)focus();
                if (s && s->tl()) {
                    if (s->tl()->maximized()) {
                        s->tl()->unsetMaximizedRequest();
                    } else {
                        s->tl()->setMaximizedRequest();
                    }
                }
                return;
            }

            default:
                break;
            }
        }
    }

    G::scene()->handleKeyboardKeyEvent(event);
}

void Keyboard::focusChanged()
{
    /* Here we use the current keyboard focus client to set the topbar app name */

    LTexture *topbarTitleTexture { nullptr };

    if (focus())
    {
        Client *client = (Client*)focus()->client();

        if (client->app && client->app->nameTexture)
            topbarTitleTexture = client->app->nameTexture;
        else
            topbarTitleTexture = G::textures()->defaultTopbarAppName;
    }
    else
    {
        topbarTitleTexture = G::textures()->defaultTopbarAppName;
    }

    for (Output *output : G::outputs())
    {
        output->topbar.appName.setTexture(topbarTitleTexture);
        output->topbar.update();
    }
}
