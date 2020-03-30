# LED-clock
LED clock with two 4-Digit seven segment displays and RGB pixels for seconds 


The clock LED colors can be changed with the potentiometer on the front. First half gives contrasting colors, second half turn is monochromatic.

A brief push of the button toggles the outer LEDs off and on, and dims the time/date displays.

A 1.5 second push enters clock setting mode. Display blinks year field. Brief push to increment value. Long push to accept and move on to next field.

Order of fields:

YEAR
MONTH
DAY OF MONTH
HOURS (0-23)
MINUTES

After accepting MINUTES, new time will be displayed, another long hold sets the clock and the “:’ on time begins to block again. if you kill power before then, adjustments will be ignored and previous time stored in RTC will be used.
I powered this with a 5V, 700mA charge from an old flip phone that I added a DC barrel connector to.


Code
https://github.com/martianzero/LED-clock/blob/master/ds3231_TM1637_Clock_2020_ESP8622_DOUBLE_DISPLAY_NANO_Dotstar_FN.ino

Additional Libraries used
https://github.com/avishorp/TM1637
https://github.com/adafruit/RTClib
https://github.com/adafruit/Adafruit_DotStar

Link to pics of clock and circuit diagram
https://imgur.com/a/LZ0mJUH
