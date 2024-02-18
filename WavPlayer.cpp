/* I2S Example

    This example code will output 100Hz sine wave and triangle wave to 2-channel of I2S driver
    Every 5 seconds, it will change bits_per_sample [16, 24, 32] for i2s data

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
    NOTE: This code current only works with SDFS, the standard SD library does not work
*/
// set TEST to 1 to create sine and triangle waves for testing
#define TEST 0

#include <math.h>

#include "RFconfig.h"
#include <driver/i2s.h>
//#include <freertos/queue.h>

#if USE_STANDARD_SD
#include "SD.h"
#else
#include <SdFatConfig.h>
#include <sdfat.h>
#endif


#define SAMPLE_RATE     (36000)
#define I2S_NUM         (0)
#define WAVE_FREQ_HZ    (100)
#define PI              (3.14159265)
#define I2S_MCK_IO      (I2S_PIN_NO_CHANGE)
#define I2S_BCK_IO      (I2S_PIN_NO_CHANGE)
#define I2S_WS_IO       (I2S_PIN_NO_CHANGE)
#define I2S_DO_IO       (GPIO_NUM_32)
#define I2S_DI_IO       (I2S_PIN_NO_CHANGE)

#define SAMPLE_PER_CYCLE (SAMPLE_RATE/WAVE_FREQ_HZ)

#if TEST == 1
static void setup_triangle_sine_waves(int bits)
{
    int* samples_data = (int*)malloc(((bits + 8) / 16) * SAMPLE_PER_CYCLE * 4);
    unsigned int i, sample_val;
    double sin_float, triangle_float, triangle_step = (double)pow(2.0, bits) / SAMPLE_PER_CYCLE;
    size_t i2s_bytes_write = 0;

    printf("\r\nTest bits=%d free mem=%d, written data=%d\n", bits, esp_get_free_heap_size(), ((bits + 8) / 16) * SAMPLE_PER_CYCLE * 4);

    triangle_float = -(pow(2.0, bits) / 2 - 1);

    for (i = 0; i < SAMPLE_PER_CYCLE; i++) {
        sin_float = sin(i * PI / 180.0);
        if (sin_float >= 0)
            triangle_float += triangle_step;
        else
            triangle_float -= triangle_step;

        sin_float *= (pow(2.0, bits) / 2 - 1);

        if (bits == 16) {
            sample_val = 0;
            sample_val += (short)triangle_float;
            sample_val = sample_val << 16;
            sample_val += (short)sin_float;
            samples_data[i] = sample_val;
        }
        else if (bits == 24) { //1-bytes unused
            samples_data[i * 2] = ((int)triangle_float) << 8;
            samples_data[i * 2 + 1] = ((int)sin_float) << 8;
        }
        else {
            samples_data[i * 2] = ((int)triangle_float);
            samples_data[i * 2 + 1] = ((int)sin_float);
        }

    }

	i2s_set_clk((i2s_port_t)I2S_NUM, SAMPLE_RATE, bits, (i2s_channel_t)2);
    //Using push
    // for(i = 0; i < SAMPLE_PER_CYCLE; i++) {
    //     if (bits == 16)
    //         i2s_push_sample(0, &samples_data[i], 100);
    //     else
    //         i2s_push_sample(0, &samples_data[i*2], 100);
    // }
    // or write
    i2s_write((i2s_port_t)I2S_NUM, samples_data, ((bits + 8) / 16) * SAMPLE_PER_CYCLE * 4, &i2s_bytes_write, 100);

    free(samples_data);
}
void WavPlayer(char* wavfile)
{
    //for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes or 8-bytes each sample)
    //depend on bits_per_sample
    //using 6 buffers, we need 60-samples per buffer
    //if 2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
    //if 2-channels, 24/32-bit each channel, total buffer is 360*8 = 2880 bytes
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),                    // Only TX
        .sample_rate = (uint32_t)SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                //Interrupt level 1
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .use_apll = false,
    };
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_MCK_IO,
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO                                               //Not used
    };
    i2s_driver_install((i2s_port_t)I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin((i2s_port_t)I2S_NUM, &pin_config);

    int test_bits = 16;
    while (1) {
        setup_triangle_sine_waves(test_bits);
        vTaskDelay(5000 / portTICK_RATE_MS);
        test_bits += 8;
        if (test_bits > 32) {
            test_bits = 16;
        }
    }
}
#else

#define CCCC(c1, c2, c3, c4)    ((c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

/* these are data structures to process wav file */
typedef enum headerState_e {
    HEADER_RIFF, HEADER_FMT, HEADER_DATA, DATA
} headerState_t;

typedef struct wavRiff_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
} wavRiff_t;

typedef struct wavProperties_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wavProperties_t;
/* variables hold file, state of process wav file and wav file properties */

#if USE_STANDARD_SD
    SPIClass spiSDCard;
#else
    extern SdFs SD; // fat16/32 and exFAT
#endif

//
void debug(uint8_t* buf, int len)
{
    for (int i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print("\t");
    }
    Serial.println();
}

void WavPlayer(char* wavfile)
{
    if (*wavfile == '\0')
        return;
    headerState_t state = HEADER_RIFF;
    wavProperties_t wavProps;
    //i2s configuration 
    static i2s_config_t i2s_config = {
         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
         .sample_rate = 44100,
         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
         .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
         .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  //Interrupt level 1
         .dma_buf_count = 8,
         .dma_buf_len = 128,
         .use_apll = false
    };

    static i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_PIN_NO_CHANGE, //this is BCK pin
        .ws_io_num = I2S_PIN_NO_CHANGE, // this is LRCK pin
        .data_out_num = AUDIO_OUT_PORT, // this is DATA output pin
        .data_in_num = I2S_PIN_NO_CHANGE   //Not used
    };
    /* open wav file and process it */
	FsFile file = SD.open(wavfile);
    if (file) {
        Serial.println("file opened");
        int c = 0;
        int n;
        bool done = false;
        while (!done && file.available()) {
            switch (state) {
            case HEADER_RIFF:
                wavRiff_t wavRiff;
                n = file.read((uint8_t*)&wavRiff, sizeof(wavRiff_t));
                if (n == sizeof(wavRiff_t)) {
                    if (wavRiff.chunkID == CCCC('R', 'I', 'F', 'F') && wavRiff.format == CCCC('W', 'A', 'V', 'E')) {
                        state = HEADER_FMT;
                        Serial.println("HEADER_RIFF");
                    }
                }
                break;
            case HEADER_FMT:
                n = file.read((uint8_t*)&wavProps, sizeof(wavProperties_t));
                if (n == sizeof(wavProperties_t)) {
                    state = HEADER_DATA;
                }
                break;
            case HEADER_DATA:
                uint32_t chunkId, chunkSize;
                n = file.read((uint8_t*)&chunkId, sizeof(chunkId));
                if (n == sizeof(chunkId)) {
                    if (chunkId == CCCC('d', 'a', 't', 'a')) {
                        Serial.println("HEADER_DATA");
                    }
                }
                n = file.read((uint8_t*)&chunkSize, sizeof(chunkSize));
                if (n == sizeof(chunkSize)) {
                    Serial.println("DATA");
                    state = DATA;
                }
                //initialize i2s with configurations above
				i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
				i2s_set_pin(I2S_NUM_0, &pin_config);
                Serial.println(String("sample rate:") + wavProps.sampleRate);
                //set sample rates of i2s to sample rate of wav file
                i2s_set_sample_rates(I2S_NUM_0, wavProps.sampleRate);
                break;
                /* after processing wav file, it is time to process music data */
            case DATA:
                uint8_t data[2];
                n = file.read(data, 2);
				//Serial.println("count:" + String(n) + " data:" + data[0] + " " + data[1]);
                size_t bytes;
                i2s_write(I2S_NUM_0, (const void*)data, 2, &bytes, portMAX_DELAY);
                if (n != 2) {
					Serial.println("done due to file read error:" + String(file.getError()));
                    done = true;
                }
                break;
            }
        }
        file.close();
    }
    else {
        Serial.println("error opening " + String(wavfile));
    }
    i2s_driver_uninstall(I2S_NUM_0); //stop & destroy i2s driver 
    ledcAttachPin(AUDIO_OUT_PORT, toneChannel);
    Serial.println("finished!");
}
#endif
