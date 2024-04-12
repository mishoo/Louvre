#ifndef LKEYBOARDLEAVEEVENT_H
#define LKEYBOARDLEAVEEVENT_H

#include <LKeyboardEvent.h>
#include <LTime.h>

/**
 * @brief Event generated when a surface or view loses keyboard focus.
 */
class Louvre::LKeyboardLeaveEvent final : public LKeyboardEvent
{
public:
    /**
     * @brief Constructor for LKeyboardLeaveEvent.
     *
     * @param serial The serial number of the event.
     * @param ms The millisecond timestamp of the event.
     * @param us The microsecond timestamp of the event.
     * @param device The input device that originated the event.
     */
    LKeyboardLeaveEvent(UInt32 serial = LTime::nextSerial(), UInt32 ms = LTime::ms(),
                        UInt64 us = LTime::us(), LInputDevice *device = nullptr) noexcept:
        LKeyboardEvent(LEvent::Subtype::Leave, serial, ms, us, device)
    {}
};

#endif // LKEYBOARDLEAVEEVENT_H
