#include <ctype.h>
#include <deviceinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void toHexString(
    const uint8_t data[], const uint32_t dataLength,
    char buf[], uint32_t* bufLength)
{
    uint32_t totalLength = (6 * dataLength) + (dataLength / 8);

    if (*bufLength >= totalLength) {
        uint32_t length = 0;

        for (int i = 0; i < dataLength; i++) {
            length += snprintf(&buf[length], (*bufLength - length), "%s0x%02X%s",
                ((i % 8 == 0) && (i != 0)) ? "\n" : "",
                data[i],
                (i == dataLength - 1) ? "" : ", ");
        }

        buf[length] = '\0';

        *bufLength = length;

    } else {
        printf("ERROR: bufLength %d is too small for %d chars\n", *bufLength, totalLength);
        *bufLength = 0;
    }
}

#define Trace(fmt, ...)                                 \
    do {                                                \
        fprintf(stdout, "<< " fmt "\n", ##__VA_ARGS__); \
        fflush(stdout);                                 \
    } while (0)

void ShowMenu()
{
    printf("Enter\n"
           "\tI : Get ID as a string.\n"
           "\tB : Get binary ID.\n"
           "\tC : Get chipset\n"
           "\tF : Get firmware version\n"
           "\tA : Get architecture\n"
           "\tR : Get maximum supported resolution\n"
           "\tS : Get summary of available outputs\n");
}

int main(int argc, char* argv[])
{
    int16_t result = 0;

    ShowMenu();

    int character;
    do {
        character = toupper(getc(stdin));

        switch (character) {
        case 'B': {
            uint8_t buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, sizeof(buffer));

            result = deviceinfo_id(buffer, &bufferLength);
            if (bufferLength > 0) {
                char id[125];
                uint32_t idSize = sizeof(id);
                toHexString(buffer, bufferLength, id, &idSize);

                Trace("ID[%d]: %s", bufferLength, id);
            } else if (result == 0) {
                Trace("No ID available for this device, or instance or buffer is null");
            } else {
                Trace("Buffer too small (or invlid parameters), should be at least of size %d ", -result);
            }
            break;
        }
        case 'I': {
            char bufferstr[150];
            uint8_t bufferLength = sizeof(bufferstr);
            memset(bufferstr, 0, sizeof(bufferstr));
            result = deviceinfo_id_str(bufferstr, &bufferLength);

            if (bufferLength > 0) {
                Trace("ID[%d]: %s", bufferLength, bufferstr);
            } else if (result == 0) {
                Trace("No ID available for this device, or instance or buffer is null");
            } else {
                Trace("Buffer too small (or invlid parameters), should be at least of size %d ", -result);
            }

            break;
        }
        case 'C': {
            char buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, sizeof(buffer));

            result = deviceinfo_chipset(buffer, &bufferLength);
            if (bufferLength > 0) {
                Trace("Chipset: %s", buffer);
                memset(buffer, 0, sizeof(buffer));
            } else if (result == 0) {
                Trace("Instance or buffer is null");
            } else {
                Trace("Buffer too small, should be at least of size %d ", -result);
            }

            break;
        }
        case 'F': {
            char buffer[150];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, sizeof(buffer));

            result = deviceinfo_firmware_version(buffer, &bufferLength);
            if (bufferLength > 0) {
                Trace("Firmware Version: %s", buffer);
                memset(buffer, 0, sizeof(buffer));
            } else if (result == 0) {
                Trace("Instance or buffer is null");
            } else {
                Trace("Buffer too small, should be at least of size %d ", -result);
            }
            break;
        }
        case 'A': {
            char buffer[25];
            uint8_t bufferLength = sizeof(buffer);
            memset(buffer, 0, sizeof(buffer));

            result = deviceinfo_architecure(buffer, &bufferLength);
            if (bufferLength > 0) {
                Trace("Architecure: %s", buffer);
                memset(buffer, 0, sizeof(buffer));
            } else if (result == 0) {
                Trace("Instance or buffer is null");
            } else {
                Trace("Buffer too small, should be at least of size %d ", -result);
            }
            break;
        }
        case 'R': {
            deviceinfo_output_resolution_t res = DEVICEINFO_RESOLUTION_UNKNOWN;
            deviceinfo_maximum_output_resolution(&res);
            Trace("Output Resolution: %d", res);
            break;
        }
        case 'S': {
            bool atmos = false;
            bool hdr = false;
            bool cec = false;
            uint8_t maxLength = 0;

            deviceinfo_hdcp_t hdcp = DEVICEINFO_HDCP_UNAVAILABLE;

            deviceinfo_output_resolution_t resolutions[DEVICEINFO_RESOLUTION_LENGTH];
            memset(resolutions, 0, sizeof(resolutions));

            deviceinfo_video_output_t video_outputs[DEVICEINFO_VIDEO_LENGTH];
            memset(video_outputs, 0, sizeof(video_outputs));

            deviceinfo_audio_output_t audio_outputs[DEVICEINFO_AUDIO_LENGTH];
            memset(audio_outputs, 0, sizeof(audio_outputs));

            memset(resolutions, 0, sizeof(resolutions));

            maxLength = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_t);
            deviceinfo_output_resolutions(resolutions, &maxLength);

            maxLength = sizeof(video_outputs) / sizeof(deviceinfo_video_output_t);
            deviceinfo_video_outputs(video_outputs, &maxLength);

            maxLength = sizeof(audio_outputs) / sizeof(deviceinfo_audio_output_t);
            deviceinfo_audio_outputs(audio_outputs, &maxLength);

            deviceinfo_cec(&cec);
            deviceinfo_hdr(&hdr);
            deviceinfo_atmos(&atmos);
            deviceinfo_hdcp(&hdcp);

            Trace("Summary: atmos: %s hdr: %s, cec: %s hdcp: %d", atmos ? "available" : "unavailable", hdr ? "available" : "unavailable", cec ? "available" : "unavailable", hdcp);

            break;
        }
        case '?': {
            ShowMenu();
            break;
        }
        default:
            break;
        }
    } while (character != 'Q');

    Trace("Done");

    return 0;
}