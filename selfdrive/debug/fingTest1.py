#!/usr/bin/env python

# simple script to get a vehicle fingerprint.

# Instructions:
# - connect to a Panda
# - run selfdrive/boardd/boardd
# - launching this script
# - since some messages are published at low frequency, keep this script running for few
#   seconds, until all messages are received at least once
import os
import time
from common.basedir import BASEDIR
from common.realtime import sec_since_boot
from common.fingerprints import eliminate_incompatible_cars, all_known_cars
from selfdrive.swaglog import cloudlog
import zmq
import selfdrive.messaging as messaging
from selfdrive.services import service_list

def load_interfaces(x):
  ret = {}
  for interface in x:
    try:
      imp = __import__('selfdrive.car.%s.interface' % interface, fromlist=['CarInterface']).CarInterface
    except ImportError:
      imp = None
    for car in x[interface]:
      ret[car] = imp
  return ret

def _get_interface_names():
  # read all the folders in selfdrive/car and return a dict where:
  # - keys are all the car names that which we have an interface for
  # - values are lists of spefic car models for a given car
  interface_names = {}
  for car_folder in [x[0] for x in os.walk(BASEDIR + '/selfdrive/car')]:
    try:
      car_name = car_folder.split('/')[-1]
      model_names = __import__('selfdrive.car.%s.values' % car_name, fromlist=['CAR']).CAR
      model_names = [getattr(model_names, c) for c in model_names.__dict__.keys() if not c.startswith("__")]
      interface_names[car_name] = model_names
    except (ImportError, IOError):
      pass

  return interface_names


# imports from directory selfdrive/car/<name>/
interfaces = load_interfaces(_get_interface_names())

context = zmq.Context()
logcan = messaging.sub_sock(context, service_list['can'].port)
msgs = {}
"""""
while True:
  lc = messaging.recv_sock(logcan, True)
  for c in lc.can:
    # read also msgs sent by EON on CAN bus 0x80 and filter out the
    # addr with more than 11 bits
    if c.src%0x80 == 0 and c.address < 0x800:
      msgs[c.address] = len(c.dat)

  fingerprint = ', '.join("%d: %d" % v for v in sorted(msgs.items()))

  print "number of messages:", len(msgs)
  print "fingerprint", fingerprint
"""""
candidate_cars = all_known_cars()
finger = {}
st = None
st_passive = sec_since_boot()  # only relevant when passive
can_seen = False
i = 1000
while i:
  i = i - 1
  for a in messaging.drain_sock(logcan):
    for can in a.can:
      can_seen = True
      # ignore everything not on bus 0 and with more than 11 bits,
      # which are ussually sporadic and hard to include in fingerprints
      if can.src == 0 and can.address < 0x800:
        finger[can.address] = len(can.dat)
        candidate_cars = eliminate_incompatible_cars(can, candidate_cars)
  #print "can:", a.can
  print "candidate_cars:", candidate_cars
    
  if st is None and can_seen:
    st = sec_since_boot()          # start time
  ts = sec_since_boot()
    # if we only have one car choice and the time_fingerprint since we got our first
    # message has elapsed, exit. Toyota needs higher time_fingerprint, since DSU does not
    # broadcast immediately
  if len(candidate_cars) == 1 and st is not None:
      # TODO: better way to decide to wait more if Toyota
    time_fingerprint = 1.0 if ("TOYOTA" in candidate_cars[0] or "LEXUS" in candidate_cars[0]) else 0.1
    if (ts-st) > time_fingerprint:
      break

    # bail if no cars left or we've been waiting too long
  elif len(candidate_cars) == 0:
    print "none finger"#return None, finger
    break

   # time.sleep(0.01)

print "can_car:", candidate_cars
print "finger:", finger
candidate=candidate_cars[0]
fingerprints=finger

if candidate is None:
  print("car doesn't match any fingerprints: %r", fingerprints)
  if passive:
    candidate = "mock"
  else:
    print " None, None"

interface_cls = interfaces[candidate]

if interface_cls is None:
  print("car matched %s, but interface wasn't available or failed to import" % candidate)
  print " None, None"

params = interface_cls.get_params(candidate, fingerprints)

print "para:", params
