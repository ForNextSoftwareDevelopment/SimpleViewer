#include "Utils.h"

namespace Utils
{
    /*********************************************************************
    * Create a font name from a scalable font given a desired fontsize
    *********************************************************************/
    XFontStruct *LoadQueryScalableFont(Display *display, int screen, char *fontName, int fontSize)
    {
        int i,j, field;
        char newname[500];
        int res_x, res_y;

        if ((fontName == NULL) || (fontName[0] != '-')) return NULL;

        // calculate our screen resolution in dots per inch. 25.4mm = 1 inch
        res_x = DisplayWidth(display, screen)/(DisplayWidthMM(display, screen)/25.4);
        res_y = DisplayHeight(display, screen)/(DisplayHeightMM(display, screen)/25.4);

        // copy the font name, changing the scalable fields as we do so
        for(i = j = field = 0; fontName[i] != '\0' && field <= 14; i++)
        {
            newname[j++] = fontName[i];
            if (fontName[i] == '-')
            {
                field++;
                switch(field)
                {
                    case 7: // pixel size
                    case 12: // average width
                        // change from "-0-" to "-*-" */
                        newname[j] = '*';
                        j++;
                        if (fontName[i+1] != '\0') i++;
                        break;
                    case 8: // point size
                        // change from "-0-" to "-<size>-"
                        sprintf(&newname[j], "%d", fontSize);
                        while (newname[j] != '\0') j++;
                        if (fontName[i+1] != '\0') i++;
                        break;
                    case 9: // x-resolution
                    case 10: // y-resolution
                        // change from an unspecified resolution to res_x or res_y
                        sprintf(&newname[j], "%d", (field == 9) ? res_x : res_y);
                        while(newname[j] != '\0') j++;
                        while((fontName[i+1] != '-') && (fontName[i+1] != '\0')) i++;
                        break;
                }
            }
        }

        // if there aren’t 14 hyphens, it isn’t a well formed name
        newname[j] = '\0';
        if (field != 14) return NULL;
        return XLoadQueryFont(display, newname);
    }

    /*********************************************************************
    * Convert int to a string
    *********************************************************************/
    std::string IntToStr (int i)
    {
        const int max_size = std::numeric_limits<int>::digits10 + 1 /*sign*/ + 1 /*0-terminator*/;
        char buffer[max_size] = {0};
        sprintf(buffer, "%d", i);
        return std::string(buffer);
    };

    /*********************************************************************
    * Convert floating point to a string
    *********************************************************************/
    std::string FloatToStr (float f)
    {
        std::string strFloat;
        int iTemp;
        bool negative = false;
        bool minusset = false;
        bool smallerthenone = false;
        bool billions = false;
        bool hundredmillions = false;
        bool tenmillions = false;
        bool millions = false;
        bool hundredthousands = false;
        bool tenthousands = false;
        bool thousands = false;
        bool hundreds = false;
        bool tens = false;

        // If float is negative the add a minus sign and make it positive
        if (f < 0.0f)
        {
            negative = true;
            f = -f;
        } else strFloat.append(" ");

        // If float is larger then 10 bilion then forget it
        if (f >= 10000000000.0f) return ("Overflow");
        if (f < 1.0f) smallerthenone = true; else strFloat.append(" ");

        if (f >= 1000000000.0f)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // billions
            iTemp = f /1000000000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no billions)
            f = f - (iTemp * 1000000000);

            billions = true;
        } else strFloat.append(" ");

        if ((f >= 100000000.0f) || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // hundredmillions
            iTemp = f /100000000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no hundredmillions)
            f = f - (iTemp * 100000000);

            hundredmillions = true;
        } else strFloat.append(" ");

        if ((f >= 10000000.0f) || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // tenmillions
            iTemp = f /10000000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no tenmillions)
            f = f - (iTemp * 10000000);

            tenmillions = true;
        } else strFloat.append(" ");

        if ((f >= 1000000.0f) || tenmillions || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // millions
            iTemp = f /1000000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no millions)
            f = f - (iTemp * 1000000);

            millions = true;
        } else strFloat.append(" ");

        if ((f >= 100000.0f) || millions || tenmillions || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // hundredthousands
            iTemp = f /100000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no hundredthousands)
            f = f - (iTemp * 100000);

            hundredthousands = true;
        } else strFloat.append(" ");

        if ((f >= 10000.0f) || hundredthousands || millions || tenmillions || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // tenthousands
            iTemp = f /10000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no tenthousands)
            f = f - (iTemp * 10000);

            tenthousands = true;
        } else strFloat.append(" ");

        if ((f >= 1000.0f) || tenthousands || hundredthousands || millions || tenmillions || hundredmillions || billions)
        {
            // thousands
            iTemp = f /1000;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no thousands)
            f = f - (iTemp * 1000);

            thousands = true;
        } else strFloat.append(" ");

        if ((f >= 100.0f) || thousands || tenthousands || hundredthousands || millions || tenmillions || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // hundreds
            iTemp = f /100;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no hundreds)
            f = f - (iTemp * 100);

            hundreds = true;
        } else strFloat.append(" ");

        if ((f >= 10.0f) || hundreds || thousands || tenthousands || hundredthousands || millions || tenmillions || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // tens
            iTemp = f /10;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (no tens)
            f = f - (iTemp * 10);

            tens = true;
        } else strFloat.append(" ");

        if ((f >= 1.0f) || tens || hundreds  || thousands || tenthousands || hundredthousands || millions || tenmillions || hundredmillions || billions)
        {
            // Set minus sign if megative number and not allready set
            if (negative && !minusset)
            {
                strFloat.append("-");
                minusset = true;
            };

            // ones
            iTemp = f;
            strFloat.append(IntToStr(iTemp));

            // Calculate new f (everything after the decimal point)
            f = f - (iTemp);
        } else strFloat.append(" ");

        // Set minus sign if megative number and not allready set
        if (negative && !minusset)
        {
            strFloat.append("-");
            minusset = true;
        };

        // tend's
        if (smallerthenone) strFloat.append("0");
        iTemp = f * 10;
        strFloat.append(".");
        strFloat.append(IntToStr(iTemp));

        // Calculate new f
        f = f - (((float)iTemp) * 0.1f);

        // hundred's
        iTemp = f * 100;
        strFloat.append(IntToStr(iTemp));

        // Calculate new f
        f = f - (((float)iTemp) * 0.01f);

        // thousand's
        iTemp = f * 1000;
        strFloat.append(IntToStr(iTemp));

        // Calculate new f
        f = f - (((float)iTemp) * 0.001f);

        // tenth thousand's
        iTemp = f * 10000;
        strFloat.append(IntToStr(iTemp));

        // Calculate new f
        f = f - (((float)iTemp) * 0.0001f);

        // hundreds thousand's
        iTemp = f * 100000;
        strFloat.append(IntToStr(iTemp));

        // Calculate new f
        f = f - (((float)iTemp) * 0.00001f);

        // millionth
        iTemp = f * 1000000;
        strFloat.append(IntToStr(iTemp));

        return (strFloat);
    };

    /*********************************************************************
    * Convert a string to a floating point
    *********************************************************************/
    float StrToFloat (char *pStrFloat)
    {
        float f = 0.0;
        char *p = pStrFloat;
        bool decimalPointPresent = false;
        bool negativeNumber = false;
        int nBeforeDecimalPoint = 0;
        int nAfterDecimalPoint = 0;

        // Check for minus sign
        if (*p == '-')
        {
            negativeNumber = true;
            pStrFloat++;
            p++;
        };

        // First count the numbers before the decimal point
        while ((*p == '0') || (*p == '1') || (*p == '2') || (*p == '3') || (*p == '4') || (*p == '5') || (*p == '6') || (*p == '7') || (*p == '8') || (*p == '9'))
        {
            nBeforeDecimalPoint++;
            p++;
        };

        // Found dec point
        if (*p == '.')
        {
            decimalPointPresent = true;
            p++;
        };

        // Secondly count the numbers after the decimal point
        while ((*p == '0') || (*p == '1') || (*p == '2') || (*p == '3') || (*p == '4') || (*p == '5') || (*p == '6') || (*p == '7') || (*p == '8') || (*p == '9'))
        {
            nAfterDecimalPoint++;
            p++;
        };

        // Add number before decimal point
        for (int i=0; i<nBeforeDecimalPoint; i++)
        {
            float factor = 1.0f;
            for (int j=nBeforeDecimalPoint; j>(i+1); j--) factor = factor * 10.0f;

            f = f + factor * (*pStrFloat - '0');
            pStrFloat++;
        };

        // Add number after decimal point
        if (decimalPointPresent)
        {
            // Skip decimal point
            pStrFloat++;

            // Convert all numbers after the decimal point to float
            for (int i=0; i<nAfterDecimalPoint; i++)
            {
                float factor = 0.1f;
                for (int j=0; j<i; j++) factor = factor * 0.1f;

                f = f + factor * (*pStrFloat - '0');
                pStrFloat++;
            };
        };

        if (negativeNumber) f = -f;
        return (f);
    };

    /*********************************************************************
    * Create a random float between 0.0f and 1.0f
    *********************************************************************/
    float Random (void)
    {
        float res;
        unsigned int tmp;
        static unsigned int seed = 0xFFFF0C59;

        seed *= 16807;

        tmp = seed ^ (seed >> 4) ^ (seed << 15);

        *((unsigned int *) &res) = (tmp >> 9) | 0x3F800000;

        return (res - 1.0);
    }

    /*********************************************************************
    * Show all window property values
    *********************************************************************/
    bool showWindowProperties(Display* display, Window window, std::string propname)
    {
        int result;
        Atom property;
        Atom actual_type_return;
        int actual_format_return;
        unsigned long bytes_after_return;
        unsigned char* prop_return;
        unsigned long n_items;

        printf("-----GET_PROPERTY_VALUE-------\r\n");
        printf("\tPropname: %s\n", propname.c_str());
        property = XInternAtom(display, propname.c_str(), True);
        if(property==None)
        {
          printf("\tWrong Atom\n");
          return false;
        }

        result = XGetWindowProperty(display, window, property, 0, (~0L), False, AnyPropertyType, &actual_type_return, &actual_format_return, &n_items, &bytes_after_return, &prop_return);
        if (result != Success)
        {
            printf("\tXGetWindowProperty failed\r\n");
            return (false);
        } else
        {
            printf("\tActual Type: %s\r\n", XGetAtomName(display, actual_type_return));
            printf("\tProperty format: %d\r\n", actual_format_return);
            printf("\tByte after return: %ld\r\n", bytes_after_return);
            printf("\tnitems return: %ld\r\n", n_items);
            for (unsigned int i=0; i<n_items; i++)
            {
                printf("\tprop: %s\r\n", XGetAtomName(display,((Atom*)prop_return)[i]));
            }
        }

        printf("-----END OF GET_PROPERTY_VALUE-------\r\n\r\n");

        return (true);
    }
}
