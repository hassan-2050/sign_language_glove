#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <last1_inferencing.h>

uint8_t newMACAddress[] = {0x58, 0xBF, 0x25, 0xA1, 0x25, 0x48};

typedef struct struct_message {
    char a[92];
    int id;
} struct_message;

struct_message myData;

String board1Data = "";
String board2Data = "";
float features[8580];
unsigned long aggregatedCount = 0;
bool runInference = false;
bool isSystemBusy = false;  // Added flag to check if system is busy

int splitString(const String &str, char delimiter, float result[]) {
    int index = 0;
    int pos = 0;
    String tempStr = str;
    while ((pos = tempStr.indexOf(delimiter)) != -1) {
        String part = tempStr.substring(0, pos);
        result[index++] = part.toFloat();
        tempStr.remove(0, pos + 1);
    }
    if (tempStr.length() > 0) {
        result[index++] = tempStr.toFloat();
    }
    return index;
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (isSystemBusy) {
        return;  // Ignore the data if the system is busy
    }

    memcpy(&myData, incomingData, sizeof(myData));
    String s = myData.a;

    if (s.endsWith("a")) {
        s.remove(s.length() - 1);
        board1Data = s;
    } else if (s.endsWith("b")) {
        s.remove(s.length() - 1);
        board2Data = s;
    }

    if (board1Data.length() > 0 && board2Data.length() > 0) {
        float board1Values[11];
        float board2Values[11];

        int count1 = splitString(board1Data, ',', board1Values);
        int count2 = splitString(board2Data, ',', board2Values);

        for (int i = 0; i < count1; i++) {
            features[aggregatedCount++] = board1Values[i];
        }

        for (int i = 0; i < count2; i++) {
            features[aggregatedCount++] = board2Values[i];
        }

        if (aggregatedCount == 8580) {
            Serial.println("Collection completed.");
            Serial.print("Number of values collected: ");
            Serial.println(aggregatedCount);
            Serial.println();

            runInference = true;  // Set flag for inference
            aggregatedCount = 0;  // Reset count
        }

        board1Data = "";
        board2Data = "";
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    if (runInference) {
        isSystemBusy = true;  // Mark system as busy
ei_printf("Edge Impulse standalone inferencing (Arduino)\n");
   
    ei_impulse_result_t result = { 0 };

    // the features are stored into flash, and we don't want to load everything into RAM
    signal_t features_signal;
    features_signal.total_length = sizeof(features) / sizeof(features[0]);
    features_signal.get_data = &raw_feature_get_data;

    // invoke the impulse
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
    if (res != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", res);
        return;
    }

    // print inference return code
    ei_printf("run_classifier returned: %d\r\n", res);
    print_inference_result(result);

    runInference = false; // Reset flag after inference
    delay(10000);
    Serial.println(" data collection for new gesture");
    delay(1000);
    Serial.println("start gesture");

        isSystemBusy = false;  // Mark system as not busy after inference
    }
}

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void print_inference_result(ei_impulse_result_t result) {
    // Print how long it took to perform inference
    ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
            result.timing.dsp,
            result.timing.classification,
            result.timing.anomaly);

    // Print the prediction results (object detection)
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    ei_printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) {
            continue;
        }
        ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                bb.label,
                bb.value,
                bb.x,
                bb.y,
                bb.width,
                bb.height);
    }
    // Print the prediction results (classification)
#else
    ei_printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }
#endif

    // Print anomaly result (if it exists)
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif
}




