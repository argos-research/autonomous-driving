/***************************************************************************

    created              : Sat Mar 18 23:16:38 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: human.cpp 5522 2013-06-17 21:03:25Z torcs-ng $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file

    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: human.cpp 5522 2013-06-17 21:03:25Z torcs-ng $
*/

/* 2013/3/21 Tom Low-Shang
 *
 * Moved original contents of
 *
 * drivers/human/human.cpp,
 * drivers/human/human.h,
 * drivers/human/pref.cpp,
 * drivers/human/pref.h,
 *
 * to libs/robottools/rthumandriver.cpp.
 *
 * CMD_* defines from pref.h are in interfaces/playerpref.h.
 *
 * Robot interface entry points are still here.
 */

//#define __DEBUG__PARKING
#define listen_addr "0.0.0.0"
#define listen_port 9002

#include <humandriver.h>

#include <gpsSensor.h>
#include <obstacleSensors.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <state.pb.h>
#include <control.pb.h>

#include <netinet/tcp.h>

static ObstacleSensors *sens;
static tTrack	*curTrack;
static bool autonomous;

static HumanDriver robot("human");

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void drive_mt(int index, tCarElt* car, tSituation *s);
static void drive_at(int index, tCarElt* car, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void resumerace(int index, tCarElt* car, tSituation *s);
static int  pitcmd(int index, tCarElt* car, tSituation *s);

static GPSSensor gps = GPSSensor();

static int sockfd, newsockfd;

#ifdef _WIN32
/* Must be present under MS Windows */
BOOL WINAPI DllEntryPoint (HINSTANCE hDLL, DWORD dwReason, LPVOID Reserved)
{
    return TRUE;
}
#endif


static void
shutdown(const int index)
{
  close(newsockfd);
  close(sockfd);
    robot.shutdown(index);
}//shutdown


/**
 *
 *	InitFuncPt
 *
 *	Robot functions initialisation.
 *
 *	@param pt	pointer on functions structure
 *  @return 0
 */
static int
InitFuncPt(int index, void *pt)
{
	tRobotItf *itf = (tRobotItf *)pt;

    robot.init_context(index);

	itf->rbNewTrack = initTrack;	/* give the robot the track view called */
	/* for every track change or new race */
	itf->rbNewRace  = newrace;
	itf->rbResumeRace  = resumerace;

	/* drive during race */
	itf->rbDrive = robot.uses_at(index) ? drive_at : drive_mt;
	itf->rbShutdown = shutdown;
	itf->rbPitCmd   = pitcmd;
	itf->index      = index;

	return 0;
}//InitFuncPt


/**
 *
 * moduleWelcome
 *
 * First function of the module called at load time :
 *  - the caller gives the module some information about its run-time environment
 *  - the module gives the caller some information about what he needs
 * MUST be called before moduleInitialize()
 *
 * @param	welcomeIn Run-time info given by the module loader at load time
 * @param welcomeOut Module run-time information returned to the called
 * @return 0 if no error occured, not 0 otherwise
 */
extern "C" int
moduleWelcome(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut)
{
	welcomeOut->maxNbItf = robot.count_drivers();

	return 0;
}//moduleWelcome


/**
 *
 * moduleInitialize
 *
 * Module entry point
 *
 * @param modInfo	administrative info on module
 * @return 0 if no error occured, -1 if any error occured
 */
extern "C" int
moduleInitialize(tModInfo *modInfo)
{
    return robot.initialize(modInfo, InitFuncPt);
}//moduleInitialize


/**
 * moduleTerminate
 *
 * Module exit point
 *
 * @return 0
 */
extern "C" int
moduleTerminate()
{
        robot.terminate();

	return 0;
}//moduleTerminate


/**
 * initTrack
 *
 * Search under robots/human/cars/<carname>/<trackname>.xml
 *
 * @param index
 * @param track
 * @param carHandle
 * @param carParmHandle
 * @param s situation provided by the sim
 *
 */
static void
initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s)
{
    robot.init_track(index, track, carHandle, carParmHandle, s);
    curTrack = track;
}//initTrack


/**
 *
 * newrace
 *
 * @param index
 * @param car
 * @param s situation provided by the sim
 *
 */
void
newrace(int index, tCarElt* car, tSituation *s)
{
    sockfd = socket(AF_INET,SOCK_STREAM,0);

    /* make port reusable */
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct sockaddr_in srv_addr, cli_addr;
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    struct hostent *host;
    host = gethostbyname(listen_addr); // listen globally...
    if (host == NULL) {
      printf("Couldn't resolve hostname!\n");
    }
    bcopy((char *)host->h_addr, (char *)&srv_addr.sin_addr.s_addr, host->h_length);
    srv_addr.sin_port = htons(listen_port); // ... on port 9002

    while (bind(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
      printf("bind failed!\n");
    }

    /* wait for S/A VM to connect */
    listen(sockfd, 5);
    socklen_t clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    /* since we need to be fast -> disable nagle's algorithm
     * https://en.wikipedia.org/wiki/Nagle's_algorithm
     */
    int yes = 1;
    int result = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));

    robot.new_race(index, car, s);
    /* add laser obstacle sensors */
    sens = new ObstacleSensors(curTrack, car);

    /* (-car->_cimension_x/2, car->dimension_y/2)
     *      +-----L-----+
     *      B     0     F
     *      +-----R-----+ (car->_cimension_x/2, -car->dimension_y/2)
     */
    /* car, angle, move_x, move_y, range */
    sens->addSensor(car, 0, car->_dimension_x/2, 0, 20); // front
    sens->addSensor(car, 90, car->priv.wheel[2].relPos.x, -car->_dimension_y/2, 20); // right
    sens->addSensor(car, 180, -car->_dimension_x/2, 0, 20); // back
}//newrace


void
resumerace(int index, tCarElt* car, tSituation *s)
{
    robot.resume_race(index, car, s);
}

/*
 * Function
 *
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *
 */
static void
drive_mt(int index, tCarElt* car, tSituation *s)
{
    robot.drive_mt(index, car, s);
}//drive_mt


/*
 * Function
 *
 *
 * Description
 *
 *
 * Parameters
 *
 *
 * Return
 *
 *
 * Remarks
 *
 */
static void
drive_at(int index, tCarElt* car, tSituation *s)
{
    /* update gps position */
    gps.update(car);
    vec2 axlePos = { (car->_pos_X + car->priv.wheel[2].relPos.x + car->_pos_X + car->priv.wheel[3].relPos.x) / 2,
                     (car->_pos_Y + car->priv.wheel[2].relPos.y + car->_pos_Y + car->priv.wheel[3].relPos.y) / 2
                   };
    vec2 myPos = gps.getPosition();
    //printf("Players's position according to GPS is (%f, %f)\n", myPos.x, myPos.y);

    /* update laser proximity sensors */
    sens->sensors_update(s);

    /* actuators */
    protobuf::State current_state;
    current_state.set_steer(car->_steerCmd);
    current_state.set_accelcmd(car->_accelCmd);
    current_state.set_brakecmd(car->_brakeCmd);
    current_state.set_timestamp(s->deltaTime);
    /* wheels */
    protobuf::Wheel* wheels[4];
    for(int i = 0; i < 4; i++) {
      wheels[i] = current_state.add_wheel();
      wheels[i]->set_spinvel(car->_wheelSpinVel(i));
    }
    /* specification protobuf */
    protobuf::Specification* spec = current_state.mutable_specification();
    spec->set_length(car->_dimension_x);
    spec->set_width(car->_dimension_y);
    spec->set_wheelradius(car->_wheelRadius(0));
    spec->set_steerlock(car->_steerLock);
    /* sensors
     * add laser proximity sensors as specified in newrace
     */
    std::list<SingleObstacleSensor> sensors_list = sens->getSensorsList();
    for(std::list<SingleObstacleSensor>::iterator it = sensors_list.begin(); it != sensors_list.end(); ++it) {
      protobuf::Sensor* sensor = current_state.add_sensor();
      sensor->set_type(protobuf::Sensor_SensorType_LASER);
      sensor->add_value((*it).getDistance());
    }
    /* add single gps sensor */
    protobuf::Sensor* sensor = current_state.add_sensor();
    sensor->set_type(protobuf::Sensor_SensorType_GPS);
    sensor->add_value(axlePos.x);
    sensor->add_value(axlePos.y);

    /* prepare protobuf message (serialize, calculate length, ...) */
    uint32_t message_length = 0;
    std::string output;
    current_state.SerializeToString(&output);
    message_length = htonl(output.size());

    /* send protobuf message to S/A VM
     * 1. message length as uint32_t
     * 2. message (State) itself
     */
    write(newsockfd, &message_length, 4);
    write(newsockfd, output.c_str(), output.length());

    #ifdef __DEBUG__PARKING
    printf("State was sent, waiting for control msg... ");
    #endif

    /* receive protobuf message from S/A VM
     * 1. message length as uint32_t
     * 2. message (Control) itself
     */
    read(newsockfd, &message_length, 4);
    message_length = ntohl(message_length);         // get length of message
    char* buffer = malloc(message_length);          // alloc buffer for message
    read(newsockfd, buffer, message_length);

    #ifdef __DEBUG__PARKING
    printf("done\n");
    #endif

    protobuf::Control control;
    control.ParseFromArray(buffer, message_length); // parse protobuf into control

    printf("steer: %f brake: %f accel: %f speed: %f autonomous: %d\n",
    control.steer(),
    control.brakecmd(),
    control.accelcmd(),
    control.speed(),
    control.autonomous());

    if (control.autonomous()) {
      autonomous = true;
      printf("AUTONOMOUS!\n");
      car->_steerCmd = control.steer();
      if (control.speed() < 0) { // negative speed wanted
        car->_clutchCmd = 1;
        car->_gearCmd = -1;
        car->_clutchCmd = 0;
        if (car->_speed_x < control.speed()) { // car too fast
          car->_brakeCmd = 1.0;
          car->_accelCmd = 0.0;
        } else { // car too slow
          car->_accelCmd = 1.0;
          car->_brakeCmd = 0.0;
        }
      } else { // positive speed
        car->_clutchCmd = 1;
        car->_gearCmd = 1;
        car->_clutchCmd = 0;
        if (car->_speed_x > control.speed()) { // car too fast
          car->_brakeCmd = 1.0;
          car->_accelCmd = 0.0;
        } else {
          car->_accelCmd = 1.0;
          car->_brakeCmd = 0.0;
        }
      }
    } else {
      printf("CONVENTIAL!\n");
      robot.drive_at(index, car, s);
      if (car->_accelCmd != 0.0 || car->_brakeCmd != 0.0)
        autonomous = false;

      if (autonomous) // if user didn't overtake control after parking...
        car->_brakeCmd = 1.0; //... full brakes (obv. for safety reasons)
    }
}//drive_at


static int
pitcmd(int index, tCarElt* car, tSituation *s)
{
    return robot.pit_cmd(index, car, s);
}//pitcmd
