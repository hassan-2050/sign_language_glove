#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t newMACAddress[] = {0x58, 0xBF, 0x25, 0xA1, 0x25, 0x48};

typedef struct struct_message {
    char a[92];
    int id;
} struct_message;

struct_message myData;

String board1Data = "";
String board2Data = "";

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
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
    // Combine data from both boards
    String combinedData = board1Data + "," + board2Data;
    
    // Split combined data, skipping blank values
    String combinedArray[200]; // adjust the size as needed
    int combinedCount = splitString(combinedData, ',', combinedArray);

    // Merge data back
    String mergedData = joinArray(combinedArray, combinedCount, ",");

    // Print the merged data
    Serial.println(mergedData);

    // Clear the data for next readings
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
  // Empty loop
}

int splitString(const String &str, char delimiter, String result[]) {
  int index = 0;
  int pos = 0;
  String tempStr = str;
  while ((pos = tempStr.indexOf(delimiter)) != -1) {
    String part = tempStr.substring(0, pos);
    if (part.length() > 0) {
      // Only add non-blank parts to the result
      result[index++] = part;
    }
    tempStr.remove(0, pos + 1);
  }
  // Don't forget the remaining part
  if (tempStr.length() > 0) {
    result[index++] = tempStr;
  }
  return index;
}

String joinArray(const String arr[], int count, const String &delimiter) {
  String result;
  for (int i = 0; i < count; i++) {
    if (i > 0) result += delimiter;
    result += arr[i];
  }
  return result;
}
