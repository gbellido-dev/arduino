#include <AccelStepper.h>
#include <micro_ros_arduino.h>
#include <ESP32Servo.h>
#include <sensor_msgs/msg/joint_state.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/bool.h>
#include <trajectory_msgs/msg/joint_trajectory.h>
#include <trajectory_msgs/msg/joint_trajectory_point.h>
#include <builtin_interfaces/msg/duration.h>             // si usas time_from_start
#include <std_msgs/msg/header.h>                         // si usas header con frame_id

#include <rosidl_runtime_c/string_functions.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <SoftwareSerial.h>


//#define DEFAULT_WIFI_SSID "LHAB40"
//#define DEFAULT_WIFI_PASSWORD "^m*PrqJ36#Elm3!d"

#define DEFAULT_WIFI_SSID "WLAN_5464"
#define DEFAULT_WIFI_PASSWORD "44330860q"

#define DEFAULT_IP_ADDRESS "172.17.17.81"
#define DEFAULT_GATEWAY "172.17.16.1"
#define DEFAULT_SUBNET_MASK "255.255.254.0"
#define DEFAULT_DNS_SERVER "172.17.18.2"

// Pines
#define STEP_PIN1 16
#define DIR_PIN1 17
#define STEP_PIN2 5
#define DIR_PIN2 21
#define STEP_PIN3 22
#define DIR_PIN3 23
#define SERVO_PIN 26

#define RX1 16         // OK
#define TX1 17         // OK
#define RX2 4         // OK
#define TX2 5         // OK
#define RX3 18        // OK
#define TX3 19        // OK


#define PULSES_PER_REV 3200
#define GEAR_RATIO_1 30.0
#define GEAR_RATIO_2 30.0
#define GEAR_RATIO_3 30.0
#define PI 3.14159265358979323846

IPAddress local_ip;
IPAddress gateway;
IPAddress subnet;
IPAddress dns;

AccelStepper stepper1(AccelStepper::DRIVER, STEP1, DIR1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP2, DIR2);
AccelStepper stepper3(AccelStepper::DRIVER, STEP3, DIR3);
Servo gripper_servo;

SoftwareSerial Serial3(RX3, TX3);

// ROS
rcl_subscription_t subscriber;
trajectory_msgs__msg__JointTrajectory traj_msg;
//trajectory_msgs__msg__JointTrajectory * traj_msg;

void init_traj_msg_() {
  Serial.println("[init_traj_msg] Inicializando estructura traj_msg...");
  if (!trajectory_msgs__msg__JointTrajectory__init(&traj_msg)) {
    Serial.println("❌ No se pudo inicializar traj_msg");
  }

  // Inicializar secuencias internas: joint_names y points
  traj_msg.joint_names.size = 4;
  traj_msg.joint_names.capacity = 4;
  traj_msg.joint_names.data = (rosidl_runtime_c__String*)malloc(4 * sizeof(rosidl_runtime_c__String));
  if (traj_msg.joint_names.data == NULL) {
    Serial.println("❌ malloc falló para traj_msg.joint_names.data");
  }

  for (int i = 0; i < 4; i++) {
    if (!rosidl_runtime_c__String__init(&traj_msg.joint_names.data[i])) {
      Serial.printf("❌ Error al inicializar joint_names[%d]\n", i);
    }
  }

  // Reservar memoria para 1 punto de trayectoria
  traj_msg.points.size = 1;
  traj_msg.points.capacity = 1;
  traj_msg.points.data = (trajectory_msgs__msg__JointTrajectoryPoint*)malloc(sizeof(trajectory_msgs__msg__JointTrajectoryPoint));
  if (traj_msg.points.data == NULL) {
    Serial.println("❌ malloc falló para traj_msg.points.data");
  }

    // Inicializar el contenido del punto
  memset(&traj_msg.points.data[0], 0, sizeof(trajectory_msgs__msg__JointTrajectoryPoint));

  // Inicializar secuencias internas del punto con memoria válida
  traj_msg.points.data[0].positions.size = 4;
  traj_msg.points.data[0].positions.capacity = 4;
  traj_msg.points.data[0].positions.data = (double *)malloc(4 * sizeof(double));
  
  traj_msg.points.data[0].velocities.size = 4;
  traj_msg.points.data[0].velocities.capacity = 4;
  traj_msg.points.data[0].velocities.data = (double *)malloc(4 * sizeof(double));

  traj_msg.points.data[0].accelerations.size = 4;
  traj_msg.points.data[0].accelerations.capacity = 4;
  traj_msg.points.data[0].accelerations.data = (double *)malloc(4 * sizeof(double));

  traj_msg.points.data[0].effort.size = 4;
  traj_msg.points.data[0].effort.capacity = 4;
  traj_msg.points.data[0].effort.data = (double *)malloc(4 * sizeof(double));


  Serial.println("✅ traj_msg completamente inicializado.");
}

void init_traj_msg() {
  Serial.println("[init_traj_msg] Inicializando estructura traj_msg...");
  if (!trajectory_msgs__msg__JointTrajectory__init(&traj_msg)) {
    Serial.println("❌ No se pudo inicializar traj_msg");
    return;
  }

  traj_msg.joint_names.size = 4;
  traj_msg.joint_names.capacity = 4;
  traj_msg.joint_names.data = (rosidl_runtime_c__String*)malloc(4 * sizeof(rosidl_runtime_c__String));

  const char* names[] = {"joint1", "joint2", "joint3", "gripper"};
  for (int i = 0; i < 4; i++) {
    rosidl_runtime_c__String__init(&traj_msg.joint_names.data[i]);
    traj_msg.joint_names.data[i].data = (char*)malloc(20);
    strcpy(traj_msg.joint_names.data[i].data, names[i]);
    traj_msg.joint_names.data[i].size = strlen(names[i]);
    traj_msg.joint_names.data[i].capacity = 20;
  }

  // Header
  traj_msg.header.frame_id.data = (char *)malloc(20);
  strcpy(traj_msg.header.frame_id.data, "");
  traj_msg.header.frame_id.size = 0;
  traj_msg.header.frame_id.capacity = 20;

  traj_msg.points.size = 1;
  traj_msg.points.capacity = 1;
  traj_msg.points.data = (trajectory_msgs__msg__JointTrajectoryPoint*)malloc(sizeof(trajectory_msgs__msg__JointTrajectoryPoint));
  trajectory_msgs__msg__JointTrajectoryPoint__init(&traj_msg.points.data[0]);

  size_t N = 4;
  traj_msg.points.data[0].positions.data = (double *)malloc(N * sizeof(double));
  traj_msg.points.data[0].positions.size = N;
  traj_msg.points.data[0].positions.capacity = N;

  // Solo si los vas a usar
  traj_msg.points.data[0].velocities.data = NULL;
  traj_msg.points.data[0].accelerations.data = NULL;
  traj_msg.points.data[0].effort.data = NULL;

  Serial.println("✅ traj_msg completamente inicializado.");
}




/*

ros2 topic pub /joint_trajectory trajectory_msgs/msg/JointTrajectory "{
  joint_names: ['joint1', 'joint2', 'joint3', 'gripper_joint'],
  points: [{
    positions: [0.1, 0.2, 0.3, 0.4],
    velocities: [],
    accelerations: [],
    effort: [],
    time_from_start: {sec: 1}
  }]
}"

*/


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
  /*long s1 = readStepCount(Serial1);
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
  }*/
}


void joint_callback(const void *msgin) {
  const auto *msg = (const trajectory_msgs__msg__JointTrajectory *)msgin;
  Serial.println("-> joint_callback called!");
  if (msg->points.size > 0 && msg->points.data[0].positions.size >= 4) {

    // Mostrar los datos recibidos
    Serial.println("📨 Nueva trayectoria recibida:");
    for (size_t i = 0; i < msg->points.data[0].positions.size; i++) {
      Serial.print("  Posición[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(msg->points.data[0].positions.data[i], 4);  // con 4 decimales
    }

    // Asignar los valores recibidos
    portENTER_CRITICAL(&mux);
    target1 = msg->points.data[0].positions.data[0] * PULSES_PER_REV * GEAR_RATIO_1 / (2 * PI);
    target2 = msg->points.data[0].positions.data[1] * PULSES_PER_REV * GEAR_RATIO_2 / (2 * PI);
    target3 = msg->points.data[0].positions.data[2] * PULSES_PER_REV * GEAR_RATIO_3 / (2 * PI);
    target_gripper = msg->points.data[0].positions.data[3];
    portEXIT_CRITICAL(&mux);

    gripper_servo.write(target_gripper * 180);

    // Mostrar los valores transformados (útil para ver qué se está usando internamente)
    Serial.println("🎯 Objetivos internos:");
    Serial.print("  target1: "); Serial.println(target1);
    Serial.print("  target2: "); Serial.println(target2);
    Serial.print("  target3: "); Serial.println(target3);
    Serial.print("  target_gripper: "); Serial.println(target_gripper, 4);
  }
}


void controlTask(void *) {
  while (true) {
    portENTER_CRITICAL(&mux);
    stepper1.moveTo(target1);
    stepper2.moveTo(target2);
    stepper3.moveTo(target3);
    gripper_servo.write(target_gripper * 180);
    portEXIT_CRITICAL(&mux);

    stepper1.run();
    stepper2.run();
    stepper3.run();
    delay(1);
  }
}



void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(">> Inicio de setup");

  init_traj_msg();

  // Motores paso a paso
  Serial.println("Configurando motores...");
  stepper1.setMaxSpeed(1000); stepper1.setAcceleration(200);
  stepper2.setMaxSpeed(1000); stepper2.setAcceleration(200);
  stepper3.setMaxSpeed(1000); stepper3.setAcceleration(200);

  // Servo
  Serial.println("Inicializando servo...");
  gripper_servo.attach(SERVO_PIN, 500, 2400);
  gripper_servo.write(0);
  delay(1000);
  Serial.println("Servo listo.");

  // Puertos serie para drivers
  Serial.println("Iniciando puertos serie...");
  //Serial1.begin(115200, SERIAL_8N1, RX1, TX1);
  //Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
  //Serial3.begin(115200);
  Serial.println("Puertos serie iniciados.");

  // Configuración IP
  Serial.println("Configurando IP estática...");
  local_ip.fromString(DEFAULT_IP_ADDRESS);
  gateway.fromString(DEFAULT_GATEWAY);
  subnet.fromString(DEFAULT_SUBNET_MASK);
  dns.fromString(DEFAULT_DNS_SERVER);
  //WiFi.config(local_ip, gateway, subnet, dns);
  Serial.println("IP configurada.");

  // micro-ROS WiFi
  Serial.println("Conectando con micro-ROS Agent...");
  set_microros_wifi_transports(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD, "192.168.0.200", 8888);
  delay(2000);
  Serial.println("micro-ROS WiFi configurado.");

  // Inicialización de micro-ROS
  Serial.println("Inicializando allocator y soporte...");
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  Serial.println("Allocator y soporte listos.");

  Serial.println("Inicializando nodo ROS...");
  rclc_node_init_default(&node, "dualcore_jointstate_node", "", &support);
  Serial.println("Nodo inicializado.");

  // Suscripción
  Serial.println("Inicializando suscripción a /joint_trajectory...");
  rclc_subscription_init_default(&subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(trajectory_msgs, msg, JointTrajectory),
    "/joint_trajectory");
  Serial.println("Suscripción lista.");

  // Publicador de estados
  Serial.println("Inicializando publicador /joint_states...");
  rclc_publisher_init_default(&joint_state_pub, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
    "/joint_states");
  Serial.println("Publicador listo.");

  // Nombres de articulaciones
  Serial.println("Asignando nombres a articulaciones...");
  const char *names[] = {"joint1", "joint2", "joint3", "gripper_joint"};
  joint_state_msg.name.capacity = 4;
  joint_state_msg.name.size = 4;
  joint_state_msg.name.data = (rosidl_runtime_c__String*)malloc(4 * sizeof(rosidl_runtime_c__String));

  if (joint_state_msg.name.data == NULL) {
    Serial.println("❌ malloc falló para joint_state_msg.name.data");
    return;
  }

  for (int i = 0; i < 4; i++) {
    joint_state_msg.name.data[i].data = (char*)malloc(20);
    if (joint_state_msg.name.data[i].data == NULL) {
      Serial.print("❌ malloc falló para name.data["); Serial.print(i); Serial.println("]");
      return;
    }
    strcpy(joint_state_msg.name.data[i].data, names[i]);
    joint_state_msg.name.data[i].capacity = 20;
  }

  joint_state_msg.position.capacity = 4;
  joint_state_msg.position.size = 4;
  joint_state_msg.position.data = (double*)malloc(4 * sizeof(double));
  if (joint_state_msg.position.data == NULL) {
    Serial.println("❌ malloc falló para joint_state_msg.position.data");
    return;
  }

  Serial.println("Nombres y posición inicializados.");

  // Publicadores individuales por articulación
  Serial.println("Inicializando publicadores por articulación...");
  for (int i = 0; i < 3; i++) {
    char topic[32];
    sprintf(topic, "/joint_error/joint%d", i + 1);
    rclc_publisher_init_default(&joint_error_pub[i], &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32), topic);
    sprintf(topic, "/joint_enabled/joint%d", i + 1);
    rclc_publisher_init_default(&joint_enabled_pub[i], &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), topic);
    sprintf(topic, "/joint_blocked/joint%d", i + 1);
    rclc_publisher_init_default(&joint_blocked_pub[i], &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), topic);
  }
  Serial.println("Publicadores completados.");

  Serial.print("🧠 Heap libre antes del temporizador: ");
  Serial.println(ESP.getFreeHeap());

  // Timer
  Serial.println("Inicializando temporizador...");
  //rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(100), timer_callback);
  rcl_ret_t ret_timer = rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(500), timer_callback);
  if (ret_timer != RCL_RET_OK) {
    Serial.print("❌ Error en rclc_timer_init_default: ");
    Serial.println(ret_timer);  // imprimirá el código de error
  } else {
    Serial.println("Temporizador listo.");
  }

  // Executor
  Serial.println("Inicializando executor...");
  executor = rclc_executor_get_zero_initialized_executor();
  rcl_ret_t ret_exec_init = rclc_executor_init(&executor, &support.context, 2, &allocator);
    if (ret_exec_init != RCL_RET_OK) {
    Serial.print("❌ Error en rclc_executor_init: ");
    Serial.println(ret_exec_init);  // imprimirá el código de error
  } else {
    Serial.println("Executor inicializado correctamente.");
  }

  

  rcl_ret_t ret_add_sub = rclc_executor_add_subscription(&executor, &subscriber, &traj_msg, &joint_callback, ON_NEW_DATA);
    if (ret_add_sub != RCL_RET_OK) {
    Serial.print("❌ Error en rclc_executor_add_subscription: ");
    Serial.println(ret_add_sub);  // imprimirá el código de error
  } else {
    Serial.println("Subscripcion registrada en executor.");
  }

  rcl_ret_t ret_add_timer = rclc_executor_add_timer(&executor, &timer);
    if (ret_add_timer != RCL_RET_OK) {
    Serial.print("❌ Error en rclc_executor_add_timer: ");
    Serial.println(ret_add_timer);  // imprimirá el código de error
  } else {
    Serial.println("Temporizador registrada en executor.");
  }

  Serial.println("Executor listo.");

  // Tarea de control
  Serial.println("Creando tarea de control...");
  BaseType_t task_created = xTaskCreatePinnedToCore(controlTask, "ControlTask", 4096, NULL, 1, NULL, 1);
  if (task_created == pdPASS) {
    Serial.println("✅ Tarea de control creada correctamente.");
  } else {
    Serial.println("❌ Error al crear la tarea de control.");
  }
  Serial.println("✅ setup() finalizado correctamente.");

  Serial.println(">>> PRUEBA MANUAL DEL CALLBACK <<<");

  // Crear mensaje de prueba (dummy)
  trajectory_msgs__msg__JointTrajectory dummy;
  dummy.points.size = 1;
  dummy.points.capacity = 1;
  dummy.points.data = (trajectory_msgs__msg__JointTrajectoryPoint*)malloc(sizeof(trajectory_msgs__msg__JointTrajectoryPoint));

  dummy.points.data[0].positions.size = 4;
  dummy.points.data[0].positions.capacity = 4;
  dummy.points.data[0].positions.data = (double*)malloc(4 * sizeof(double));

  dummy.points.data[0].positions.data[0] = 0.1;
  dummy.points.data[0].positions.data[1] = 0.2;
  dummy.points.data[0].positions.data[2] = 0.3;
  dummy.points.data[0].positions.data[3] = 0.8;

  // Llamar directamente al callback
  joint_callback(&dummy);

  Serial.println(">>> CALLBACK EJECUTADO MANUALMENTE <<<");
}

const char* rclc_ret_to_string(rcl_ret_t ret) {
  switch(ret) {
    case RCL_RET_OK: return "RCL_RET_OK";
    case RCL_RET_ERROR: return "RCL_RET_ERROR";
    case RCL_RET_TIMEOUT: return "RCL_RET_TIMEOUT";
    case RCL_RET_BAD_ALLOC: return "RCL_RET_BAD_ALLOC";
    default: return "UNKNOWN";
  }
}



void loop() {
  rcl_ret_t ret = rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  if (ret != RCL_RET_OK) {
    Serial.print("❌ Error en executor: ");
    Serial.print(ret);
    Serial.print(" -> ");
    Serial.println(rclc_ret_to_string(ret));
    delay(100);  // Evita spam masivo del log
  }
}
