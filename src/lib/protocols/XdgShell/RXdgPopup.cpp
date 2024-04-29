#include <protocols/XdgShell/xdg-shell.h>
#include <protocols/XdgShell/RXdgPopup.h>
#include <protocols/XdgShell/RXdgSurface.h>
#include <protocols/XdgShell/RXdgPositioner.h>
#include <private/LPointerPrivate.h>
#include <private/LPopupRolePrivate.h>
#include <private/LSurfacePrivate.h>
#include <private/LFactory.h>
#include <LKeyboard.h>
#include <LCompositor.h>
#include <LClient.h>
#include <LLog.h>

using namespace Louvre::Protocols::XdgShell;

static const struct xdg_popup_interface imp
{
    .destroy = &RXdgPopup::destroy,
    .grab = &RXdgPopup::grab,
#if LOUVRE_XDG_WM_BASE_VERSION >= 3
    .reposition = &RXdgPopup::reposition
#else
    .reposition = NULL
#endif
};

RXdgPopup::RXdgPopup
(
    RXdgSurface *xdgSurfaceRes,
    RXdgSurface *xdgParentSurfaceRes,
    RXdgPositioner *xdgPositionerRes,
    UInt32 id
)
    :LResource
    (
        xdgSurfaceRes->client(),
        &xdg_popup_interface,
        xdgSurfaceRes->version(),
        id,
        &imp
    ),
    m_xdgSurfaceRes(xdgSurfaceRes)
{
    xdgSurfaceRes->m_xdgPopupRes.reset(this);

    LPopupRole::Params popupRoleParams
    {
        this,
        xdgSurfaceRes->surface(),
        &xdgPositionerRes->m_positioner
    };

    m_popupRole.reset(LFactory::createObject<LPopupRole>(&popupRoleParams));
    xdgSurfaceRes->surface()->imp()->setParent(xdgParentSurfaceRes->surface());
    xdgSurfaceRes->surface()->imp()->setPendingRole(popupRole());
}

RXdgPopup::~RXdgPopup()
{
    compositor()->onAnticipatedObjectDestruction(popupRole());

    if (popupRole()->surface())
    {
        for (LSurface *child : popupRole()->surface()->children())
        {
            if (child->popup() && child->mapped())
            {
                wl_resource_post_error(
                    xdgSurfaceRes()->resource(),
                    XDG_WM_BASE_ERROR_NOT_THE_TOPMOST_POPUP,
                    "The client tried to map or destroy a non-topmost popup.");
            }
        }

        popupRole()->surface()->imp()->setKeyboardGrabToParent();
        popupRole()->surface()->imp()->setMapped(false);
    }
}

/******************** REQUESTS ********************/

void RXdgPopup::destroy(wl_client */*client*/, wl_resource *resource)
{
    wl_resource_destroy(resource);
}

void RXdgPopup::grab(wl_client */*client*/, wl_resource *resource, wl_resource */*seat*/, UInt32 serial)
{
    auto &res { *static_cast<RXdgPopup*>(wl_resource_get_user_data(resource)) };

    if (!res.popupRole()->surface())
    {
        LLog::warning("[RXdgPopup::grab] XDG Popup keyboard grab request without surface. Ignoring it.");
        return;
    }

    const LEvent *triggeringEvent { res.client()->findEventBySerial(serial) };

    if (!triggeringEvent)
    {
        LLog::warning("[RXdgPopup::grab] XDG Popup keyboard grab request without valid event serial. Ignoring it.");
        res.popupRole()->dismiss();
        return;
    }

    // TODO use LWeak
    res.popupRole()->grabKeyboardRequest(*triggeringEvent);

    // Check if the user accepted the grab
    if (seat()->keyboard()->grab() != res.popupRole()->surface())
        res.popupRole()->dismiss();
}

#if LOUVRE_XDG_WM_BASE_VERSION >= 3
void RXdgPopup::reposition(wl_client */*client*/, wl_resource *resource, wl_resource *positioner, UInt32 token)
{
    RXdgPopup &res { *static_cast<RXdgPopup*>(wl_resource_get_user_data(resource)) };

    if (!res.popupRole()->surface() || !res.popupRole()->surface()->popup())
    {
        LLog::warning("[RXdgPopup::grab] XDG Popup keyboard grab request without surface. Ignoring it.");
        return;
    }

    RXdgPositioner &xdgPositionerRes { *static_cast<RXdgPositioner*>(wl_resource_get_user_data(positioner)) };

    if (!xdgPositionerRes.validate())
        return;

    res.popupRole()->imp()->positioner = xdgPositionerRes.m_positioner;
    res.popupRole()->imp()->repositionToken = token;
    res.popupRole()->imp()->stateFlags.add(LPopupRole::LPopupRolePrivate::HasPendingReposition | LPopupRole::LPopupRolePrivate::CanBeConfigured);
    res.popupRole()->configureRequest();
    if (!res.popupRole()->imp()->stateFlags.check(LPopupRole::LPopupRolePrivate::HasConfigurationToSend))
        res.popupRole()->configure(res.popupRole()->calculateUnconstrainedRect());
}
#endif

/******************** EVENTS ********************/

void RXdgPopup::configure(const LRect &rect) noexcept
{
    xdg_popup_send_configure(resource(), rect.x(), rect.y(), rect.w(), rect.h());
}

void RXdgPopup::popupDone() noexcept
{
    xdg_popup_send_popup_done(resource());
}

bool RXdgPopup::repositioned(UInt32 token) noexcept
{
#if LOUVRE_XDG_WM_BASE_VERSION >= 3
    if (version() >= 3)
    {
        xdg_popup_send_repositioned(resource(), token);
        return true;
    }
#endif
    L_UNUSED(token);
    return false;
}
