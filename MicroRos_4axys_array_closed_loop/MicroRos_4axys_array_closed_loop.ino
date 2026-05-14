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

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();} }

#define LED_PIN 27
#define PI 3.14159265358979323846
#define SERIAL_TIMEOUT 100 // ms

#define DEFAULT_WIFI_SSID "WLAN_5464"
#define DEFAULT_WIFI_PASSWORD "44330860q"
#define DEFAULT_IP_ADDRESS "192.168.0.77"
#define DEFAULT_GATEWAY "192.168.0.1"
#define DEFAULT_SUBNET_MASK "255.255.255.0"
#define DEFAULT_DNS_SERVER "192.168.0.200"

#define DEFAULT_ROS_AGENT_IP "192.168.0.200"
#define DEFAULT_ROS_AGENT_UDP_PORT 8888

// UART único
#define UART_RX 19
#define UART_TX 18

// Motores - Pines
#define STEP_PIN1 16
#define DIR_PIN1 17
#define STEP_PIN2 5
#define DIR_PIN2 21
#define STEP_PIN3 22
#define DIR_PIN3 23
#define SERVO_PIN 26

// Direcciones de los motores
#define ADDR1 0xE0
#define ADDR2 0xE1
#define ADDR3 0xE2

// Comandos
#define CMD_STEP_COUNT  0x33
#define CMD_POSITION_ERROR 0x39
#define CMD_ENABLED 0x3A
#define CMD_BLOCKED 0x3E

#define PULSES_PER_REV 3200
#define GEAR_RATIO 1.0 // Relación de engranaje

IPAddress local_ip, gateway, subnet, dns;

AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN1, DIR_PIN1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_PIN2, DIR_PIN2);
AccelStepper stepper3(AccelStepper::DRIVER, STEP_PIN3, DIR_PIN3);
Servo gripper_servo;

HardwareSerial &MotorSerial = Serial1; // Solo usaremos Serial1

const byte motor_addresses[3] = {ADDR1, ADDR2, ADDR3};

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile long target1 = 0, target2 = 0, target3 = 0;
volatile float target_gripper = 0.0;
unsigned long last_gripper_update = 0;
float current_gripper = 0.0;
const float gripper_step = 0.01;
const int gripper_interval_ms = 14;

unsigned long last_debug = 0;
const int debug_interval_ms = 1000;

long steps[3] = {0, 0, 0};
float errors[3] = {0.0, 0.0, 0.0};

// ROS
rcl_subscription_t subscriber;
rcl_publisher_t joint_state_pub;
rcl_publisher_t joint_error_pub[3];
rcl_publisher_t joint_enabled_pub[3];
rcl_publisher_t joint_blocked_pub[3];

sensor_msgs__msg__JointState joint_state_msg;
std_msgs__msg__Float32 joint_error_msg[3];
std_msgs__msg__Bool joint_enabled_msg[3];
std_msgs__msg__Bool joint_blocked_msg[3];
std_msgs__msg__Float32MultiArray traj_msg;

rcl_allocator_t allocator;
rclc_support_t support;
rcl_node_t node;
rclc_executor_t executor;
rcl_timer_t timer;

bool micro_ros_init_successful;
enum states { WAITING_AGENT, AGENT_AVAILABLE, AGENT_CONNECTED, AGENT_DISCONNECTED } state;

// -------------------------------------------------------------------
// FUNCIONES DE COMUNICACIÓN UART CON MOTORES
// -------------------------------------------------------------------

int32_t sendCommandAndRead(byte address, byte cmd, byte expectedBytes, bool isSigned) {
  byte checksum = address + cmd;
  MotorSerial.flush();
  while (MotorSerial.available()) MotorSerial.read(); // limpiar buffer

  MotorSerial.write(address);
  MotorSerial.write(cmd);
  MotorSerial.write(checksum);

  unsigned long startTime = millis();
  while (MotorSerial.available() < expectedBytes + 1) {
    if (millis() - startTime > SERIAL_TIMEOUT) {
      Serial.print("⚠️ Timeout UART 0x");Serial.print(address, HEX);Serial.print(" CMD 0x");Serial.println(cmd, HEX);
      return 0;
    }
  }

  byte response[5];
  for (int i = 0; i < expectedBytes + 1; i++) {
    response[i] = MotorSerial.read();
  }

  if (expectedBytes == 2) {
    return isSigned ? (int16_t)((response[1] << 8) | response[2]) : (uint16_t)((response[1] << 8) | response[2]);
  } else if (expectedBytes == 4) {
    return isSigned ? (int32_t)((response[1] << 24) | (response[2] << 16) | (response[3] << 8) | response[4])
                    : (uint32_t)((response[1] << 24) | (response[2] << 16) | (response[3] << 8) | response[4]);
  }
  return 0;
}

long readStepCount(byte motor_addr) {
  return sendCommandAndRead(motor_addr, CMD_STEP_COUNT, 4, true);
}

float readError(byte motor_addr) {
  int16_t err = sendCommandAndRead(motor_addr, CMD_POSITION_ERROR, 2, true);
  return err / 182.0;
}

bool readEnabled(byte motor_addr) {
  int32_t en = sendCommandAndRead(motor_addr, CMD_ENABLED, 1, false);
  return en == 1;
}

bool readBlocked(byte motor_addr) {
  int32_t blk = sendCommandAndRead(motor_addr, CMD_BLOCKED, 1, false);
  return blk == 1;
}

// -------------------------------------------------------------------
// CONTROL DE MOVIMIENTO
// -------------------------------------------------------------------

void controlTask(void *) {
  while (true) {
    portENTER_CRITICAL(&mux);
    float local_target1 = target1;
    float local_target2 = target2;
    float local_target3 = target3;
    float local_target_gripper = target_gripper;
    portEXIT_CRITICAL(&mux);

    stepper1.moveTo(local_target1);
    stepper2.moveTo(local_target2);
    stepper3.moveTo(local_target3);
    stepper1.run();
    stepper2.run();
    stepper3.run();


    unsigned long now = millis();
    if (now - last_gripper_update >= gripper_interval_ms) {
      last_gripper_update = now;
      if (abs(current_gripper - local_target_gripper) > gripper_step) {
        current_gripper += (current_gripper < local_target_gripper) ? gripper_step : -gripper_step;
        gripper_servo.write(current_gripper * 180.0);
      }
    }
    now = millis();
    if (now - last_debug >= debug_interval_ms) {
      last_debug = now;
      Serial.print("stepper1.moveT: "); Serial.println(local_target1);
      Serial.print("stepper1.curre: "); Serial.println(stepper1.currentPosition());
      Serial.print("stepper1.steps: "); Serial.println(steps[0]);
      Serial.print("stepper1.error: "); Serial.println(errors[0]);

      Serial.print("stepper2.moveT: "); Serial.println(local_target2);
      Serial.print("stepper2.curre: "); Serial.println(stepper2.currentPosition());
      Serial.print("stepper2.steps: "); Serial.println(steps[1]);
      Serial.print("stepper2.error: "); Serial.println(errors[1]);

      Serial.print("stepper3.moveT: "); Serial.println(local_target3);
      Serial.print("stepper3.curre: "); Serial.println(stepper3.currentPosition());
      Serial.print("stepper3.steps: "); Serial.println(steps[2]);
      Serial.print("stepper3.error: "); Serial.println(errors[2]);
    }

    vTaskDelay(1);
  }
}

// -------------------------------------------------------------------
// CALLBACKS DE ROS
// -------------------------------------------------------------------

void joint_callback(const void *msgin) {
  const std_msgs__msg__Float32MultiArray *msg = (const std_msgs__msg__Float32MultiArray *) msgin;
  if (msg->data.size >= 4) {
    portENTER_CRITICAL(&mux);
    target1 = msg->data.data[0] * (PULSES_PER_REV/360.0) * GEAR_RATIO;
    target2 = msg->data.data[1] * (PULSES_PER_REV/360.0) * GEAR_RATIO;
    target3 = msg->data.data[2] * (PULSES_PER_REV/360.0) * GEAR_RATIO;
    target_gripper = msg->data.data[3];
    portEXIT_CRITICAL(&mux);
  }
}


void timer_callback(rcl_timer_t *, int64_t) {
  
  for (int i = 0; i < 3; i++) {
    steps[i] = readStepCount(motor_addresses[i]);
    errors[i] = readError(motor_addresses[i]);
  }

  for (int i = 0; i < 3; i++) {
    joint_state_msg.position.data[i] = (float)steps[i] * (360.0 / (PULSES_PER_REV * GEAR_RATIO)); ;
  }
  joint_state_msg.position.data[3] = target_gripper;

  rcl_publish(&joint_state_pub, &joint_state_msg, NULL);

  for (int i = 0; i < 3; i++) {
    joint_error_msg[i].data = errors[i];
    //joint_enabled_msg[i].data = readEnabled(motor_addresses[i]);
    //joint_blocked_msg[i].data = readBlocked(motor_addresses[i]);

    rcl_publish(&joint_error_pub[i], &joint_error_msg[i], NULL);
    //rcl_publish(&joint_enabled_pub[i], &joint_enabled_msg[i], NULL);
    //rcl_publish(&joint_blocked_pub[i], &joint_blocked_msg[i], NULL);
  }
}

// -------------------------------------------------------------------
// SETUP Y LOOP PRINCIPAL
// -------------------------------------------------------------------

void error_loop(){
  Serial.println("error_loop()");
  while(1){ delay(100); }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[setup] Comenzando setup...");
  Serial.println("[setup] Inicializando puertos UART...");
  MotorSerial.begin(19200, SERIAL_8N1, UART_RX, UART_TX);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);
  state = WAITING_AGENT;

  Serial.println("[setup] Configurando motores...");
  stepper1.setMaxSpeed(3200); stepper1.setAcceleration(1000);
  stepper2.setMaxSpeed(3200); stepper2.setAcceleration(1000);
  stepper3.setMaxSpeed(3200); stepper3.setAcceleration(1000);

  Serial.println("[setup] Inicializando servo...");
  gripper_servo.attach(SERVO_PIN, 500, 2500);
  gripper_servo.write(0);
  delay(1000);

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

// --- El loop gestionará conexión ROS como antes

#define EXECUTE_EVERY_N_MS(MS, X)  do { \
  static volatile int64_t init = -1; \
  if (init == -1) { init = uxr_millis();} \
  if (uxr_millis() - init > MS) { X; init = uxr_millis();} \
} while (0)

bool create_entities()
{
  Serial.println("[ROS] Connected to agent... calling create_entities()");
  Serial.println("[ROS] Inicializando micro-ROS (allocator y soporte)...");
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  RCCHECK(rclc_node_init_default(&node, "dualcore_jointstate_node", "", &support));
  Serial.println("[ROS] Nodo ROS inicializado.");

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

  Serial.println("[ROS] Inicializando publicadores individuales...");
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

  Serial.println("[ROS] Inicializando temporizador...");
  RCCHECK(rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(1000), timer_callback));

  Serial.println("[ROS] Inicializando executor...");
  RCCHECK(rclc_executor_init(&executor, &support.context, 5, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &traj_msg, &joint_callback, ON_NEW_DATA));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));
  Serial.println("[ROS] Configuracion micro-ROS finalizada.");
  return true;
}

void destroy_entities()
{
  Serial.println("[ROS] Disconnected from agent... calling destroy_entities()");
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

void loop() {
  switch (state) {
    case WAITING_AGENT:
      EXECUTE_EVERY_N_MS(500, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_AVAILABLE : WAITING_AGENT;);
      break;
    case AGENT_AVAILABLE:
      state = create_entities() ? AGENT_CONNECTED : WAITING_AGENT;
      if (state == WAITING_AGENT) destroy_entities();
      break;
    case AGENT_CONNECTED:
      EXECUTE_EVERY_N_MS(200, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) ? AGENT_CONNECTED : AGENT_DISCONNECTED;);
      if (state == AGENT_CONNECTED) rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      break;
    case AGENT_DISCONNECTED:
      destroy_entities();
      state = WAITING_AGENT;
      break;
  }

  digitalWrite(LED_PIN, state == AGENT_CONNECTED ? 1 : 0);
}
