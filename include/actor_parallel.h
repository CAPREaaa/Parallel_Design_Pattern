static void createInitialActor(int);
static void workerCode();
static void control();
static void RoadJunction();
static void Vehicle();
static void loadRoadMap(char *);
static int initVehicles();
static int activateRandomVehicle();
static int activateVehicle(enum VehicleType);
static void handleVehicleUpdate(int);
static int planRoute(int, int, struct JunctionStruct *, int, int);
static void writeDetailedInfo();

