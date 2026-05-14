#include <AccelStepper.h>
#include <micro_ros_arduino.h>
#include <ESP32Servo.h>
#include <sensor_msgs/msg/joint_state.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/float32_multi_array.h>
#include <std_msgs/msg/bool.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <SoftwareSerial.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }

#define LED_PIN 13


#define EXECUTE_EVERY_N_MS(MS, X)  do { \
  static volatile int64_t init = -1; \
  if (init == -1) { init = uxr_millis();} \
  if (uxr_millis() - init > MS) { X; init = uxr_millis();} \
} while (0)\

void error_loop(){
  Serial.println("error_loop()");
  while(1){ delay(100); }
}

#define DEFAULT_WIFI_SSID "WLAN_5464"
#define DEFAULT_WIFI_PASSWORD "44330860q"
#define DEFAULT_IP_ADDRESS "192.168.0.77"
#define DEFAULT_GATEWAY "192.168.0.1"
#define DEFAULT_SUBNET_MASK "255.255.255.0"
#define DEFAULT_DNS_SERVER "192.168.0.200"

#define DEFAULT_ROS_AGENT_IP "192.168.0.200"
#define DEFAULT_ROS_AGENT_UDP_PORT 8888


// Pines
#define STEP1 26
#define DIR1  25
#define STEP2 33
#define DIR2  32
#define STEP3 27
#define DIR3  14
#define SERVO_PIN 21
#define RX1 19
#define TX1 18
#define RX2 4
#define TX2 5
#define RX3 16
#define TX3 17

#define PULSES_PER_REV 3200
#define GEAR_RATIO_1 30.0
#define GEAR_RATIO_2 30.0
#define GEAR_RATIO_3 30.0
#define PI 3.14159265358979323846

IPAddress local_ip, gateway, subnet, dns;
AccelStepper stepper1(AccelStepper::DRIVER, STEP1, DIR1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP2, DIR2);
AccelStepper stepper3(AccelStepper::DRIVER, STEP3, DIR3);
Servo gripper_servo;
SoftwareSerial Serial3(RX3, TX3);

rcl_subscription_t subscriber;
std_msgs__msg__Float32MultiArray traj_msg;
rcl_publisher_t joint_state_pub;
rcl_publisher_t joint_error_pub[3];
rcl_publisher_t joint_enabled_pub[3];
rcl_publisher_t joint_blocked_pub[3];
sensor_msgs__msg__JointState joint_state_msg;
std_msgs__msg__Float32 joint_error_msg[3];
std_msgs__msg__Bool joint_enabled_msg[3];
std_msgs__msg__Bool joint_blocked_msg[3];
rcl_allocator_t allocator;
rclc_support_t support;
rcl_node_t node;
rclc_executor_t executor;
rcl_timer_t timer;

bool micro_ros_init_successful;

enum states {
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED
} state;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile long target1 = 0, target2 = 0, target3 = 0;
volatile float target_gripper = 0.0;

long readStepCount(Stream &serial) {
  uint8_t cmd[] = {0xe0, 0x33, 0x13};
  serial.write(cmd, sizeof(cmd));
  delay(3);
  if (serial.available() >= 6) {
    uint8_t b[6];
    for (int i = 0; i < 6; i++) b[i] = serial.read();
    return ((int32_t)b[2] << 24) | ((int32_t)b[3] << 16) | ((int32_t)b[4] << 8) | b[5];
  }
  return 0;
}

float readError(Stream &serial) {
  uint8_t cmd[] = {0xe0, 0x39, 0x19};
  serial.write(cmd, sizeof(cmd));
  delay(3);
  if (serial.available() >= 4) {
    uint8_t b[4];
    for (int i = 0; i < 4; i++) b[i] = serial.read();
    int16_t err = (b[2] << 8) | b[3];
    return err / 182.0;  // ≈ grados
  }
  return 0.0;
}

bool readEnabled(Stream &serial) {
  uint8_t cmd[] = {0xe0, 0x3a, 0x1a};
  serial.write(cmd, sizeof(cmd));
  delay(3);
  if (serial.available() >= 3) {
    uint8_t b[3];
    for (int i = 0; i < 3; i++) b[i] = serial.read();
    return b[1] == 0x01;
  }
  return false;
}

bool readBlocked(Stream &serial) {
  uint8_t cmd[] = {0xe0, 0x3e, 0x1e};
  serial.write(cmd, sizeof(cmd));
  delay(3);
  if (serial.available() >= 3) {
    uint8_t b[3];
    for (int i = 0; i < 3; i++) b[i] = serial.read();
    return b[1] == 0x01;
  }
  return false;
}

void timer_callback(rcl_timer_t *, int64_t) {
  long s1 = readStepCount(Serial1);
  long s2 = readStepCount(Serial2);
  long s3 = readStepCount(Serial3);

  joint_state_msg.position.data[0] = (float)s1 / (PULSES_PER_REV * GEAR_RATIO_1) * 2 * PI;
  joint_state_msg.position.data[1] = (float)s2 / (PULSES_PER_REV * GEAR_RATIO_2) * 2 * PI;
  joint_state_msg.position.data[2] = (float)s3 / (PULSES_PER_REV * GEAR_RATIO_3) * 2 * PI;
  joint_state_msg.position.data[3] = target_gripper;

  rcl_publish(&joint_state_pub, &joint_state_msg, NULL);

  float err[] = {
    readError(Serial1), readError(Serial2), readError(Serial3)
  };
  bool en[] = {
    readEnabled(Serial1), readEnabled(Serial2), readEnabled(Serial3)
  };
  bool blk[] = {
    readBlocked(Serial1), readBlocked(Serial2), readBlocked(Serial3)
  };

  for (int i = 0; i < 3; i++) {
    joint_error_msg[i].data = err[i];
    joint_enabled_msg[i].data = en[i];
    joint_blocked_msg[i].data = blk[i];
    rcl_publish(&joint_error_pub[i], &joint_error_msg[i], NULL);
    rcl_publish(&joint_enabled_pub[i], &joint_enabled_msg[i], NULL);
    rcl_publish(&joint_blocked_pub[i], &joint_blocked_msg[i], NULL);
  }
}

void joint_callback(const void *msgin) {
  const std_msgs__msg__Float32MultiArray *msg = (const std_msgs__msg__Float32MultiArray *) msgin;
  //Serial.println("→ Recibido Float32MultiArray:");
  /*for (size_t i = 0; i < msg->data.size; i++) {
    Serial.print("  ["); Serial.print(i); Serial.print("] ");
    Serial.println(msg->data.data[i], 4);
  }*/

  if (msg->data.size >= 4) {
    portENTER_CRITICAL(&mux);
    target1 = msg->data.data[0] * PULSES_PER_REV * GEAR_RATIO_1 / (2 * PI);
    target2 = msg->data.data[1] * PULSES_PER_REV * GEAR_RATIO_2 / (2 * PI);
    target3 = msg->data.data[2] * PULSES_PER_REV * GEAR_RATIO_3 / (2 * PI);
    target_gripper = msg->data.data[3];
    portEXIT_CRITICAL(&mux);

    // Mostrar los valores transformados (útil para ver qué se está usando internamente)
    /*Serial.println("🎯 Objetivos internos:");
    Serial.print("  target1: "); Serial.println(target1);
    Serial.print("  target2: "); Serial.println(target2);
    Serial.print("  target3: "); Serial.println(target3);*/
    //Serial.print("  target_gripper: "); Serial.println(target_gripper, 4);
  }
}

unsigned long last_gripper_update = 0;
const int gripper_interval_ms = 14;
float current_gripper = 0.0;
const float gripper_step = 0.01;


void controlTask(void *) {
  while (true) {
    portENTER_CRITICAL(&mux);
    stepper1.moveTo(target1);
    stepper2.moveTo(target2);
    stepper3.moveTo(target3);
    //gripper_servo.write(target_gripper * 180);
    float local_target_gripper = target_gripper;
    portEXIT_CRITICAL(&mux);

    stepper1.run();
    stepper2.run();
    stepper3.run();
    unsigned long now = millis();
    if (now - last_gripper_update >= gripper_interval_ms) {
      last_gripper_update = now;

      if (abs(current_gripper - local_target_gripper) > gripper_step) {
        if (current_gripper < local_target_gripper) {
          current_gripper += gripper_step;
          if (current_gripper > local_target_gripper)
            current_gripper = local_target_gripper;
        } else {
          current_gripper -= gripper_step;
          if (current_gripper < local_target_gripper)
            current_gripper = local_target_gripper;
        }
        gripper_servo.write(current_gripper * 180.0);
      }
    }

    vTaskDelay(1);  // o yield() si lo prefieres
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[setup] Comenzando setup...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
  state = WAITING_AGENT;

  Serial.println("[setup] Configurando motores...");
  stepper1.setMaxSpeed(1000); stepper1.setAcceleration(200);
  stepper2.setMaxSpeed(1000); stepper2.setAcceleration(200);
  stepper3.setMaxSpeed(1000); stepper3.setAcceleration(200);

  Serial.println("[setup] Inicializando servo...");
  gripper_servo.attach(SERVO_PIN, 500, 2500);
  gripper_servo.write(0);
  delay(1000);

  Serial.println("[setup] Inicializando puertos UART...");
  Serial1.begin(115200, SERIAL_8N1, RX1, TX1);
  Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  Serial3.begin(115200);

  Serial.println("[setup] Configurando IP estática...");
  local_ip.fromString(DEFAULT_IP_ADDRESS);
  gateway.fromString(DEFAULT_GATEWAY);
  subnet.fromString(DEFAULT_SUBNET_MASK);
  dns.fromString(DEFAULT_DNS_SERVER);

  //Serial.println("[setup] Aplicando configuración IP estática con WiFi.config...");
  //WiFi.config(local_ip, gateway, subnet, dns);

  Serial.println("[setup] Conectando a micro-ROS Agent...");
  set_microros_wifi_transports(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD, DEFAULT_ROS_AGENT_IP, DEFAULT_ROS_AGENT_UDP_PORT);
  delay(2000);


  Serial.println("[setup] Lanzando tarea de control...");
  xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, NULL, 1, NULL, 1);

  Serial.println("[setup] Setup finalizado correctamente.");
}

bool create_entities()
{
  Serial.println("[setup] Inicializando micro-ROS (allocator y soporte)...");
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  RCCHECK(rclc_node_init_default(&node, "dualcore_jointstate_node", "", &support));
  Serial.println("[setup] Nodo ROS inicializado.");

  // Inicializar Float32MultiArray
  traj_msg.data.capacity = 4;
  traj_msg.data.size = 0;
  traj_msg.data.data = (float*) malloc(traj_msg.data.capacity * sizeof(float));
  traj_msg.layout.dim.capacity = 0;
  traj_msg.layout.dim.size = 0;
  traj_msg.layout.dim.data = NULL;
  traj_msg.layout.data_offset = 0;

  RCCHECK(rclc_subscription_init_default(
    &subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
    "/joint_positions"));

  RCCHECK(rclc_publisher_init_default(&joint_state_pub, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
    "/joint_states"));

  const char *names[] = {"joint1", "joint2", "joint3", "gripper_joint"};
  joint_state_msg.name.capacity = 4;
  joint_state_msg.name.size = 4;
  joint_state_msg.name.data = (rosidl_runtime_c__String*)malloc(4 * sizeof(rosidl_runtime_c__String));
  for (int i = 0; i < 4; i++) {
    joint_state_msg.name.data[i].data = (char*)malloc(20);
    strcpy(joint_state_msg.name.data[i].data, names[i]);
    joint_state_msg.name.data[i].capacity = 20;
  }
  joint_state_msg.position.capacity = 4;
  joint_state_msg.position.size = 4;
  joint_state_msg.position.data = (double*)malloc(4 * sizeof(double));

  Serial.println("[setup] Inicializando publicadores individuales...");
  for (int i = 0; i < 3; i++) {
    char topic[32];
    sprintf(topic, "/joint_error/joint%d", i + 1);
    RCCHECK(rclc_publisher_init_default(&joint_error_pub[i], &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), topic));
    sprintf(topic, "/joint_enabled/joint%d", i + 1);
    RCCHECK(rclc_publisher_init_default(&joint_enabled_pub[i], &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), topic));
    sprintf(topic, "/joint_blocked/joint%d", i + 1);
    RCCHECK(rclc_publisher_init_default(&joint_blocked_pub[i], &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), topic));
  }

  Serial.println("[setup] Inicializando temporizador...");
  RCCHECK(rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(500), timer_callback));

  Serial.println("[setup] Inicializando executor...");
  RCCHECK(rclc_executor_init(&executor, &support.context, 5, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &traj_msg, &joint_callback, ON_NEW_DATA));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  return true;
}

void destroy_entities()
{
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);
  // Finalizar publicadores individuales
  rcl_publisher_fini(&joint_state_pub, &node);

  for (int i = 0; i < 3; i++) {
    rcl_publisher_fini(&joint_error_pub[i], &node);
    rcl_publisher_fini(&joint_enabled_pub[i], &node);
    rcl_publisher_fini(&joint_blocked_pub[i], &node);
  }

  rcl_subscription_fini(&subscriber, &node);
  rcl_timer_fini(&timer);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

/*void loop() {
  //Serial.println("[loop] Ejecutando ciclo principal...");
  rcl_ret_t ret = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  if (ret != RCL_RET_OK) {
    Serial.print("❌ Error en executor: ");
    Serial.println(ret);
  }
  //delay(100);
}*/

void loop() {
  switch (state) {
    case WAITING_AGENT:
      EXECUTE_EVERY_N_MS(500, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_AVAILABLE : WAITING_AGENT;);
      break;
    case AGENT_AVAILABLE:
      Serial.println("[loop] AGENT_AVAILABLE...");
      state = (true == create_entities()) ? AGENT_CONNECTED : WAITING_AGENT;
      if (state == WAITING_AGENT) {
        Serial.println("[loop] WAITING_AGENT...");
        destroy_entities();
      }else if(state == AGENT_CONNECTED){
        Serial.println("[loop] AGENT_CONNECTED...");
      };
      break;
    case AGENT_CONNECTED:
      EXECUTE_EVERY_N_MS(200, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_CONNECTED : AGENT_DISCONNECTED;);
      if (state == AGENT_CONNECTED) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      }
      break;
    case AGENT_DISCONNECTED:
      Serial.println("[loop] AGENT_DISCONNECTED...");
      destroy_entities();
      state = WAITING_AGENT;
      break;
    default:
      break;
  }

  if (state == AGENT_CONNECTED) {
    digitalWrite(LED_PIN, 1);
  } else {
    digitalWrite(LED_PIN, 0);
  }
}