/***************************************************************************

    file                 : sumo.cpp
    created              : Mo 07. Dec 11:00:00 CEST 2015
    copyright            : (C) 2015 Alexander Weidinger

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <tgf.h>
#include <track.h>
#include <car.h>
#include <raceman.h>
#include <robottools.h>
#include <robot.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <humandriver.h>
#include "json.hpp"

#include <fstream>

using json = nlohmann::json;
using namespace std;

GfLogger* PLogSUMO = 0;

static HumanDriver robot("human");

static tTrack	*curTrack;

static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s);
static void newrace(int index, tCarElt* car, tSituation *s);
static void dataexchange(int index, tCarElt* car, tSituation *s);
static void drive_at(int index, tCarElt* car, tSituation *s);
static void drive_mt(int index, tCarElt* car, tSituation *s);
static void endrace(int index, tCarElt *car, tSituation *s);
static void shutdown(int index);
static int  InitFuncPt(int index, void *pt);

/* new SD2 api */
extern "C" int moduleWelcome(const tModWelcomeIn* welcomeIn, tModWelcomeOut* welcomeOut) {
  welcomeOut->maxNbItf = 1;
  return 0;
}
extern "C" int moduleInitialize(tModInfo *ModInfo) {
  PLogSUMO = GfLogger::instance("MapDumper");
  memset(ModInfo, 0, sizeof(tModInfo));

  ModInfo->name    = "MapDumper";		/* name of the module (short) */
  ModInfo->desc    = "";	/* description of the module (can be long) */
  ModInfo->fctInit = InitFuncPt;		/* init function */
  ModInfo->gfId    = ROB_IDENT;		/* supported framework version */
  ModInfo->index   = 1;

  return 0;
}
extern "C" int moduleTerminate() {
  return 0;
}

/* Module interface initialization. */
static int InitFuncPt(int index, void *pt) {
  tRobotItf *itf  = (tRobotItf *)pt;

  robot.init_context(index);

  itf->rbNewTrack = initTrack; /* Give the robot the track view called */
  /* for every track change or new race */
  itf->rbNewRace  = newrace; 	 /* Start a new race */
  itf->rbDrive    = robot.uses_at(index) ? drive_at : drive_mt; /* Drive during race */
  itf->rbEndRace  = endrace;	 /* End of the current race */
  itf->rbShutdown = shutdown;	 /* Called before the module is unloaded */
  itf->index      = index; 	 /* Index used if multiple interfaces */

  return 0;
}

/* Called for every track change or new race. */
static void initTrack(int index, tTrack* track, void *carHandle, void **carParmHandle, tSituation *s) {
  fstream f;
  f.open("/tmp/sd2_dump.txt", ios::out);

  PLogSUMO->debug("Shit is getting real!\n");
  PLogSUMO->debug("Number of segments: %d\n", track->nseg);

  trackSeg* cur = track->seg;

  while(cur->next != track->seg) {
    f << cur->vertex[0].x << "," << cur->vertex[0].y << ",";
    f << cur->vertex[1].x << "," << cur->vertex[1].y << "\n";
    cur = cur->next;
  }
  f.close();
  curTrack = track;
  *carParmHandle = NULL;

  robot.init_track(index, track, carHandle, carParmHandle, s);
}

static int sockfd;

/* Start a new race. */
static void newrace(int index, tCarElt* car, tSituation *s) {
  PLogSUMO->debug("Initializing SUMO!\n");

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(struct sockaddr_in));

  server_addr.sin_family = AF_INET;
  //server_addr.sin_addr.s_addr = inet_addr("131.159.208.114");
  server_addr.sin_addr.s_addr = inet_addr("10.0.2.208");
  server_addr.sin_port = htons(2000);

  connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));

  robot.new_race(index, car, s);
}

static void dataexchange(int index, tCarElt* car, tSituation *s) {
  //memset(&car->ctrl, 0, sizeof(tCarCtrl));

  /* calculate yaw in degrees
   * http://answers.ros.org/question/141366/convert-the-yaw-euler-angle-into-into-the-range-0-360/
   */
  double yaw = car->_yaw * 180.0 / M_PI;
  if(yaw < 0) yaw += 360.0;

  // json j;
  // j["veh0"] = {
  //   {"rpm", car->priv.enginerpm},
  //   {"speed", car->_speed_x},
  //   {"gear", car->priv.gear},
  //   {"pos", car->race.distRaced},
  //   {"trackLength", curTrack->length},
  //   {"angle", yaw},
  // };

  json j;
  j["veh0"] = {
    {"pos", car->race.distRaced},
    {"x", car->_pos_X},
    {"y", car->_pos_Y},
    {"z", car->_pos_Z},
    {"speed", car->_speed_x},
    {"gear", car->_gear},
    {"angle", yaw},
    {"trackLength", curTrack->length},
    {"rpm", car->priv.enginerpm},
    {"fsX", curTrack->seg[0].vertex[2].x},
    {"fsY", curTrack->seg[0].vertex[2].y}
  };

  write(sockfd, j.dump().c_str(), strlen(j.dump().c_str()) + 1);
}

static void drive_mt(int index, tCarElt* car, tSituation *s) {
  dataexchange(index, car, s);
  robot.drive_mt(index, car, s);
}

static void drive_at(int index, tCarElt* car, tSituation *s) {
  dataexchange(index, car, s);
  robot.drive_at(index, car, s);
}

/* End of the current race */
static void endrace(int index, tCarElt *car, tSituation *s) {
}

/* Called before the module is unloaded */
static void shutdown(int index) {
  robot.shutdown(index);
}
