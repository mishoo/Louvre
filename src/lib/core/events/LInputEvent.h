#ifndef LINPUTEVENT_H
#define LINPUTEVENT_H

#include <LEvent.h>

/**
 * @brief Base class for input events.
 */
class Louvre::LInputEvent : public LEvent
{
public:

    /**
     * @brief Sets the input device that originated the event.
     *
     * @param device If `nullptr` is passed, a generic fake input device is assigned.
     */
    void setDevice(LInputDevice *device) noexcept;

    /**
     * @brief Get the input device that originated this event.
     */
    LInputDevice *device() const noexcept
    {
        return m_device;
    }

protected:
    /// @cond OMIT
    LInputEvent(Type type, Subtype subtype, UInt32 serial, UInt32 ms, UInt64 us, LInputDevice *device) noexcept :
        LEvent(type, subtype, serial, ms, us)
    {
        setDevice(device);
    }
    LInputDevice *m_device;
    /// @endcond
};

#endif // LINPUTEVENT_H