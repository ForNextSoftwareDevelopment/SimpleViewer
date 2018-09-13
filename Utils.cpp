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
    * Convert a string to an integer
    *********************************************************************/
    int StrToInt (char *pStrInt)
    {
        int integer = 0;
        int number = 0;
        char *p = pStrInt;
        bool negativeNumber = false;

        try
        {
            // Check for minus sign
            if (*p == '-')
            {
                negativeNumber = true;
                pStrInt++;
                p++;
            };

            // Count the numbers
            while ((*p == '0') || (*p == '1') || (*p == '2') || (*p == '3') || (*p == '4') || (*p == '5') || (*p == '6') || (*p == '7') || (*p == '8') || (*p == '9'))
            {
                number++;
                p++;
            };

            // Add number before decimal point
            for (int i=0; i<number; i++)
            {
                int factor = 1;
                for (int j=number; j>(i+1); j--) factor = factor * 10;

                integer = integer + factor * (*pStrInt - '0');
                pStrInt++;
            };

            if (negativeNumber) integer = -integer;
        } catch (...)
        {
            std::string message = "Can't convert: ";
            message.append(pStrInt);
            Error::WriteLog("ERROR", "Utils::StrToInt", message.c_str());
        }

        return (integer);
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

    /*********************************************************************
    * Jpeg error handler
    *********************************************************************/
    struct jpegErrorManager
    {
        // "public" fields
        struct jpeg_error_mgr pub;

        // for return to caller
        jmp_buf setjmp_buffer;
    };

    char jpegLastErrorMsg[JMSG_LENGTH_MAX];

    void jpegErrorExit (j_common_ptr cinfo)
    {
        // cinfo->err actually points to a jpegErrorManager struct
        jpegErrorManager* pJpegErrorManager = (jpegErrorManager*) cinfo->err;

        // Create the message
        ( *(cinfo->err->format_message) ) (cinfo, jpegLastErrorMsg);

        // Jump to the setjmp point
        longjmp(pJpegErrorManager->setjmp_buffer, 1);
    }

    /*********************************************************************
    * Load image
    *********************************************************************/
    uint32_t* LoadImage(std::string path, unsigned int *width, unsigned int *height, bool resizing, bool deforming)
    {
        FILE *f;
        unsigned char a, r, g, b;
        struct jpeg_decompress_struct cinfo;

        // Output row buffer
        JSAMPARRAY pJpegBuffer;

        // physical row width in output buffer
        int row_stride;

        f = fopen(path.c_str(), "rb");
        if (!f)
        {
            std::string message = "Can't open imagefile: " + path;
            Error::WriteLog("ERROR", "Utils::LoadImage", message.c_str());
            return (NULL);
        }

        // We set up the normal JPEG error routines, then override error_exit
        jpegErrorManager jerr;
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = jpegErrorExit;

        // Establish the setjmp return context for my_error_exit to use
        if (setjmp(jerr.setjmp_buffer))
        {
            std::string message = "Can't read image: " + path;
            message.append("\r\nError: ");
            message.append(jpegLastErrorMsg);
            Error::WriteLog("ERROR", "Utils::LoadImage", message.c_str());
            jpeg_destroy_decompress(&cinfo);
            fclose(f);
            return (NULL);
        }

        // Create header info struct for jpg
        jpeg_create_decompress(&cinfo);

        // Set the file as source
        jpeg_stdio_src(&cinfo, f);

        // Read jpeg header
        bool result = jpeg_read_header(&cinfo, TRUE);
        if (result != 1)
        {
            std::string message = "Bad jpeg: " + path;
            Error::WriteLog("ERROR", "Utils::LoadImage", message.c_str());
            fclose(f);
            return (NULL);
        }

        (void) jpeg_start_decompress(&cinfo);
        unsigned int imWidth  = cinfo.output_width;
        unsigned int imHeight = cinfo.output_height;
        unsigned int pixSize  = cinfo.output_components;

        // Bitmap
        uint32_t *pBitmap = new uint32_t [imWidth * imHeight];
        unsigned char *ptr = (unsigned char *)pBitmap;

        row_stride = imWidth * pixSize;
        pJpegBuffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            (void) jpeg_read_scanlines(&cinfo, pJpegBuffer, 1);
            for (unsigned int x = 0; x < imWidth; x++)
            {
                a = 0; // alpha value is not supported on jpg
                r = pJpegBuffer[0][cinfo.output_components * x];
                if (cinfo.output_components > 2)
                {
                    g = pJpegBuffer[0][cinfo.output_components * x + 1];
                    b = pJpegBuffer[0][cinfo.output_components * x + 2];
                } else
                {
                    g = r;
                    b = r;
                }

                *ptr++ = b;
                *ptr++ = g;
                *ptr++ = r;
                *ptr++ = a;
            }
        }

        // Free memory
        if (jpeg_finish_decompress(&cinfo) == 0)
        {
            std::string message = "Can't decompress: " + path;
            Error::WriteLog("ERROR", "Utils::LoadImage", message.c_str());
            jpeg_destroy_decompress(&cinfo);
            fclose(f);
            return (0);
        }

        jpeg_destroy_decompress(&cinfo);

        // Close file
        fclose(f);

        if ((width == NULL) || (height == NULL))
        {
            return (pBitmap);
        }

        // Scale if needed
        if (resizing)
        {
            if (!deforming)
            {
                // Check resize for deformation
                float x_ratio = ((float)imWidth)/(float)*width;
                float y_ratio = ((float)imHeight)/(float)*height;
                if (y_ratio > x_ratio)
                {
                    *width = (unsigned int)((float)imWidth / y_ratio);
                }

                if (x_ratio > y_ratio)
                {
                    *height = (unsigned int)((float)imHeight / x_ratio);
                }
            }

            // Get resized picture
            uint32_t *pBitmapResized = ScaleImage(pBitmap, imWidth, imHeight, *width, *height);

            // Free memory from original bitmap
            delete (pBitmap);

            return (pBitmapResized);
        }

        return (pBitmap);
    }

    /*********************************************************************
    * Bilinear resize image
    *********************************************************************/
    uint32_t* ScaleImage(uint32_t *pBitmap, unsigned int w, unsigned int h, unsigned int w2, unsigned int h2)
    {
        uint32_t *temp = new uint32_t[w2*h2];
        uint32_t a, b, c, d;
        unsigned int x, y, index;
        float x_diff, y_diff, blue, red, green;
        uint32_t offset = 0;
        float x_ratio = ((float)(w-1))/(float)w2;
        float y_ratio = ((float)(h-1))/(float)h2;

        for (unsigned int i=0;i<h2;i++)
        {
            for (unsigned int j=0;j<w2;j++)
            {
                x = (unsigned int)(x_ratio * j);
                y = (unsigned int)(y_ratio * i);
                x_diff = (x_ratio * j) - x;
                y_diff = (y_ratio * i) - y;
                index = (y*w+x);
                a = pBitmap[index];
                b = pBitmap[index+1];
                c = pBitmap[index+w];
                d = pBitmap[index+w+1];

                // blue element
                // Yb = Ab(1-w)(1-h) + Bb(w)(1-h) + Cb(h)(1-w) + Db(wh)
                blue = (a&0xff)*(1-x_diff)*(1-y_diff) + (b&0xff)*(x_diff)*(1-y_diff) +
                       (c&0xff)*(y_diff)*(1-x_diff)   + (d&0xff)*(x_diff*y_diff);

                // green element
                // Yg = Ag(1-w)(1-h) + Bg(w)(1-h) + Cg(h)(1-w) + Dg(wh)
                green = ((a>>8)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>8)&0xff)*(x_diff)*(1-y_diff) +
                        ((c>>8)&0xff)*(y_diff)*(1-x_diff)   + ((d>>8)&0xff)*(x_diff*y_diff);

                // red element
                // Yr = Ar(1-w)(1-h) + Br(w)(1-h) + Cr(h)(1-w) + Dr(wh)
                red = ((a>>16)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>16)&0xff)*(x_diff)*(1-y_diff) +
                      ((c>>16)&0xff)*(y_diff)*(1-x_diff)   + ((d>>16)&0xff)*(x_diff*y_diff);

                temp[offset++] =
                        0xff000000 | // hardcode alpha
                        ((((uint32_t)red)<<16)&0xff0000) |
                        ((((uint32_t)green)<<8)&0xff00) |
                        ((uint32_t)blue);
            }
        }

        return temp;
    }
}
