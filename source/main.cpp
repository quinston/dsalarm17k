#include<iostream>
#include<ctime>
#include<iomanip>
#include <nds.h>

//#include <boost/thread.hpp>

#define EXTERNAL_DATA(name) \
	extern const uint8  name[]; \
	extern const uint8 name##_end[]; \
	extern const uint32 name##_size

class AlarmClock
{
public:
    AlarmClock() : isSnooze (true), soundId (0), mode (HOUR) {

        std::tm* currentTime = currentTimeAsTm();

        timeToGo = std::tm();

        timeToGo.tm_hour = (currentTime->tm_hour + 7);
        timeToGo.tm_min = (currentTime->tm_min + 45);
        if (timeToGo.tm_min > 59) {
            ++timeToGo.tm_hour;
            timeToGo.tm_min %= 60;
        }
        if (timeToGo.tm_hour > 23) {
            timeToGo.tm_hour %= 24;
        }


        delete currentTime;
    }
    bool getSnooze() {
        return isSnooze;
    }
    void toggleSnooze() {
        isSnooze = !isSnooze;
    }
    void setSnooze (bool status) {
        isSnooze = status;
    }

    unsigned short getMinute() {
        return timeToGo.tm_min;
    }
    unsigned short getHour() {
        return timeToGo.tm_hour;
    }

    void increment() {
        tm* currentTime = currentTimeAsTm();
        switch (mode) {
        case HOUR:

            if (timeToGo.tm_hour == 23) {
                timeToGo.tm_hour = 0;
            } else {
                ++timeToGo.tm_hour;
            }
            break;
        case MINUTE:
            do {
                if (timeToGo.tm_min == 59) {
                    timeToGo.tm_min = 0;
                } else {
                    ++timeToGo.tm_min;
                }
            } while ( (timeToGo.tm_min - currentTime->tm_min) % 15 != 0);
            break;
        }
        std::cout << currentTime -> tm_min << std::endl;
        delete currentTime;
    }
    void decrement() {
        tm* currentTime = currentTimeAsTm();
        switch (mode) {
        case HOUR:
            if (timeToGo.tm_hour == 0) {
                timeToGo.tm_hour = 23;
            } else {
                --timeToGo.tm_hour;
            }
            break;
        case MINUTE:
            do {
                if (timeToGo.tm_min == 0) {
                    timeToGo.tm_min = 59;
                } else {
                    --timeToGo.tm_min;
                }
            } while ( (timeToGo.tm_min - currentTime->tm_min) % 15 != 0);
            break;
        }
        delete currentTime;
    }

    enum EDITMODE {
        HOUR = 1,
        MINUTE = 2
    };
    EDITMODE getMode() {
        return mode;
    }
    void setMode (EDITMODE m) {
        mode = m;
    }
    void toggleMode() {
        mode = (mode == HOUR) ? MINUTE : HOUR;
    }
    //This function plays the alarm if the time has come and the clock
    //is not asleep
    void checkAlarm() {
        if (!isSnooze) {
            std::tm* currentTm = currentTimeAsTm();

            if (currentTm->tm_min == timeToGo.tm_min
                    && currentTm->tm_hour == timeToGo.tm_hour) {
                playAlarm();
            } else {
                stopAlarm();
            }

            delete currentTm;
        } else {
            stopAlarm();
        }
    }

private:
    bool isSnooze;
    std::tm timeToGo;
    int soundId;

    EDITMODE mode;

    void playAlarm() {
        //Use previous channel if there is one
        if (soundId) {
            soundResume(soundId);
        }
        //Play 17kHz tone at 100 volume (out of 125) with centered pan
        else {
            soundId = soundPlayPSG(DutyCycle_50, 17000, 100, 64);
        }
    }
    void stopAlarm() {
        soundPause (soundId);
    }
    std::tm* currentTimeAsTm() {
        std::time_t currentEpoch = std::time (0);
        std::tm* currentTm = std::localtime (&currentEpoch);
        return currentTm;
    }
};

volatile int frame = 0;

void Vblank()
{
    ++frame;
}

int main()
{
    AlarmClock a;
    //boost::thread t (a::checkAlarm);

    lcdMainOnBottom();
    videoSetMode (MODE_0_2D | DISPLAY_BG0_ACTIVE);
    consoleDemoInit();
    //consoleInit(0,0,BgType_Text4pp,BgSize_T_256x256,);

    /*
    EXTERNAL_DATA(17000);
    SCHANNEL_TIMER(0) = SOUND_FREQ();
    SCHANNEL_SOURCE(0) = (uint32_t) sound_17000;
    SCHANNEL_LENGTH(0) = ((int) sound_17000_end - (int) sound_17000) >> 2;
    SCHANNEL_CR(0) = SCHANNEL_ENABLE | SOUND_ONE_SHOT
    		| SOUND_8BIT | SOUND_VOL(0x3F);
    */
    soundEnable();

    while (1) {
        swiWaitForVBlank();

        consoleClear();
        if (a.getSnooze()) {

            std::cout << std::endl << std::endl << std::endl;
            std::cout << "\t" << "+" << "\t" << "+" << std::endl << std::endl;
            std::cout << "\t" << std::setw (2) << std::setfill ('0') << a.getHour()
                      << "\t" << std::setw (2) << std::setfill ('0') << a.getMinute()
                      << std::endl;
            std::cout << "\t";

            switch (a.getMode()) {
            case AlarmClock::MINUTE:
                std::cout << "  \t" << "~~";
                break;
            case AlarmClock::HOUR:
                std::cout << "~~" << "\t  ";
                break;
            }

            std::cout << "\t" << std::endl;
            std::cout << "\t" << "-" << "\t" << "-" << std::endl;

        }

        scanKeys();

        uint32_t keysPressed = keysDown();

        if (keysPressed & KEY_UP) {
            a.increment();
        } else if (keysPressed & KEY_DOWN) {
            a.decrement();
        } else if (keysPressed & KEY_LEFT || keysPressed & KEY_RIGHT) {
            a.toggleMode();
        }

        if (keysPressed & KEY_A) {
            a.toggleSnooze();

            if (a.getSnooze()) {
                powerOn(PM_BACKLIGHT_BOTTOM);
                powerOn(PM_BACKLIGHT_TOP);
            } else {
                powerOff(PM_BACKLIGHT_BOTTOM);
                powerOff(PM_BACKLIGHT_TOP);
            }
        }

        a.checkAlarm();
    }

    return 0;
}
