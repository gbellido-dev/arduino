#include <micro_ros_arduino.h>

#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <ESP32Servo.h>

#include <std_msgs/msg/float32_multi_array.h>

rcl_subscription_t subscriber;
std_msgs__msg__Float32MultiArray msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

#define DEFAULT_WIFI_SSID "WLAN_5464"
#define DEFAULT_WIFI_PASSWORD "44330860q"
#define AGENT_IP "192.168.0.200"
#define AGENT_PORT 8888

#define SERVO_PIN 13

#define RCCHECK(fn) { \
  rcl_ret_t temp_rc = fn; \
  if ((temp_rc != RCL_RET_OK)) { \
    Serial.print("Error en llamada RCCHECK: "); \
    Serial.println(#fn); \
    Serial.print("Código de error: "); \
    Serial.println(temp_rc); \
    error_loop(); \
  } \
}

#define RCSOFTCHECK(fn) { \
  rcl_ret_t temp_rc = fn; \
  if ((temp_rc != RCL_RET_OK)) { \
    Serial.print("Error en llamada RCSOFTCHECK: "); \
    Serial.println(#fn); \
    Serial.print("Código de error: "); \
    Serial.println(temp_rc); \
  } \
}

Servo gripper_servo;

void error_loop() {
  Serial.println("Entrando en error_loop()");
  while (1) {
    gripper_servo.write(0);
    delay(500);
    gripper_servo.write(90);
    delay(500);
  }
}

void subscription_callback(const void * msgin)
{
  const std_msgs__msg__Float32MultiArray * msg = (const std_msgs__msg__Float32MultiArray *)msgin;
  Serial.println("Callback recibido con mensaje Float32MultiArray");

  if (msg->data.size >= 4) {
    for (size_t i = 0; i < 4; i++) {
      Serial.print("Elemento[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(msg->data.data[i]);
    }

    int angle = constrain((int)msg->data.data[0], 0, 180);
    Serial.print("Ángulo para servo: ");
    Serial.println(angle);
    gripper_servo.write(angle);
  } else {
    Serial.print("Se recibieron menos de 4 elementos. Recibidos: ");
    Serial.println(msg->data.size);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Espera a que el puerto serie se inicialice

  Serial.println("Inicio de setup()");
  Serial.println("Inicializando servo...");
  gripper_servo.attach(SERVO_PIN, 500, 2400);
  gripper_servo.write(0);
  delay(1000);

  Serial.println("Configurando micro-ROS WiFi...");
  set_microros_wifi_transports(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD, AGENT_IP, AGENT_PORT);
  delay(2000);

  allocator = rcl_get_default_allocator();

  Serial.println("Inicializando estructura msg...");
  msg.data.capacity = 4;
  msg.data.size = 0;
  msg.data.data = (float*)malloc(msg.data.capacity * sizeof(float));
  if (msg.data.data == NULL) {
    Serial.println("Fallo al reservar memoria para msg.data.data");
    error_loop();
  }
  msg.layout.dim.capacity = 0;
  msg.layout.dim.size = 0;
  msg.layout.dim.data = NULL;
  msg.layout.data_offset = 0;

  Serial.println("Inicializando soporte...");
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  Serial.println("Inicializando nodo...");
  RCCHECK(rclc_node_init_default(&node, "micro_ros_arduino_node", "", &support));

  Serial.println("Inicializando suscriptor...");
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
    "micro_ros_arduino_subscriber"));

  Serial.println("Inicializando executor...");
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));

  Serial.println("Agregando suscriptor al executor...");
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));

  Serial.println("Setup completo. Esperando mensajes...");
}

void loop() {
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
  delay(100);
}
