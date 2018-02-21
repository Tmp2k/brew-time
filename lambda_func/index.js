// ====== MQTT constants =====
const MQTT_BROKER = "broker.shiftr.io";
const MQTT_KEY = process.env.MQTT_KEY;
const MQTT_SECRET = process.env.MQTT_SECRET;



// ====== Alexa API constants =====

// namespaces
const NAMESPACE_CONTROL = "Alexa.PowerController";
const NAMESPACE_DISCOVERY = "Alexa.Discovery";
const NAMESPACE_STATUS = "Alexa";

// discovery
const REQUEST_DISCOVER = "Discover.Request";
const RESPONSE_DISCOVER = "Discover.Response";

// control
const REQUEST_TURN_ON = "TurnOn";
const REQUEST_TURN_OFF = "TurnOff";
const RESPONSE_POWERSTATE = "powerState";

// status
const REQUEST_STATUS = "ReportState";

// device
const DEVICE_ID = process.env.DEVICE_ID;

// errors
const ERROR_UNSUPPORTED_OPERATION = "UnsupportedOperationError";
const ERROR_UNEXPECTED_INFO = "UnexpectedInformationReceivedError";



// ====== Node modules =====

//MQTT
const mqtt = require('mqtt');
//HTTP
const request = require('sync-request');




// entry point
exports.handler = function (event, context, callback) {

  log("Received Directive", event);
  var requestedNamespace = event.directive.header.namespace;
  var response = null;
  try {
    switch (requestedNamespace) {
      case NAMESPACE_DISCOVERY:
        response = handleDiscovery(event.directive);
        break;
      case NAMESPACE_CONTROL:
        response = handleControl(event.directive);
        break;
      case NAMESPACE_STATUS:
        response = handleStatus(event.directive);
        break;
      default:
        log("Error", "Unsupported namespace: " + requestedNamespace);
        response = handleUnexpectedInfo(requestedNamespace);
        break;
    }// switch
  } catch (error) {
    log("Error", error);
  }// try-catch
  callback(null, response);
}// exports.handler


var handleDiscovery = function(directive) {
  var header = createHeader(NAMESPACE_DISCOVERY, RESPONSE_DISCOVER);
  var payload = {
    "endpoints":
        [
          {
            "endpointId": DEVICE_ID,
            "friendlyName": "Kettle",
            "description": "Brew Time Kettle",
            "manufacturerName": "Steven Tomlinson",
            "displayCategories": [
              "SWITCH"
            ],
            "capabilities": [
              {
                "type": "AlexaInterface",
                "interface": "Alexa.PowerController",
                "version": "3",
                "properties": {
                  "supported": [
                    {
                      "name": "powerState"
                    }
                  ],
                  "proactivelyReported": true,
                  "retrievable": true
                }
              }
            ]
          }
        ]
      };

  var response = createEvent(header,payload);
  log("Sent Discovery: ", response);
  return response;
}// handleDiscovery


var handleStatus = function(directive) {
  var response = null;
  var requestedName = directive.header.name;
  switch (requestedName) {
    case REQUEST_STATUS :
      response = handleStatusRequest(directive);
      break;
    default:
      log("Error", "Unsupported operation" + requestedName);
      response = handleUnsupportedOperation();
      break;
  }// switch
  return response;
}// handleStatus


var handleStatusRequest = function(directive) {
      var res = request('GET', 'https://'+MQTT_KEY+':'+MQTT_SECRET+'@'+MQTT_BROKER+'/'+DEVICE_ID+'/actual');
      var state =  res.body.toString('utf-8');
      var context = createContext(NAMESPACE_CONTROL,RESPONSE_POWERSTATE,state);
      var header = createHeader(NAMESPACE_STATUS,'StateReport', directive.header.correlationToken);
      var payload = {};
      var endpoint = createEndpoint(DEVICE_ID);
      var event = createEvent(header,payload,endpoint);
      var response =  Object.assign(context, event);
      log("Sent Status Report: ", response);
      return response;
}// handleStatusRequest



var handleControl = function(directive) {
  var response = null;
  var requestedName = directive.header.name;
  switch (requestedName) {
    case REQUEST_TURN_ON :
      response = handleControlTurnOn(directive);
      break;
    case REQUEST_TURN_OFF :
      response = handleControlTurnOff(directive);
      break;
    default:
      log("Error", "Unsupported operation" + requestedName);
      response = handleUnsupportedOperation();
      break;

  }// switch
  return response;
}// handleControl


var handleControlTurnOn = function(directive) {
  pushState('ON');
  var context = createContext(NAMESPACE_CONTROL,RESPONSE_POWERSTATE,'ON');
  var header = createHeader(NAMESPACE_STATUS,'Response', directive.header.correlationToken);
  var payload = {};
  var endpoint = createEndpoint(DEVICE_ID);
  var event = createEvent(header,payload,endpoint);
  var response =  Object.assign(context, event);
  log("Sent Switch On: ", response);
  return response;
}// handleControlTurnOn


var handleControlTurnOff = function(directive) {
  pushState('OFF');
  var context = createContext(NAMESPACE_CONTROL,RESPONSE_POWERSTATE,'OFF');
  var header = createHeader(NAMESPACE_STATUS,'Response', directive.header.correlationToken);
  var payload = {};
  var endpoint = createEndpoint(DEVICE_ID);
  var event = createEvent(header,payload,endpoint);
  var response =  Object.assign(context, event);
  log("Sent Switch Off: ", response);
  return response;
}// handleControlTurnOff


var handleUnsupportedOperation = function() {
  var header = createHeader(NAMESPACE_CONTROL,ERROR_UNSUPPORTED_OPERATION);
  var payload = {};
  return createEvent(header,payload);
}// handleUnsupportedOperation


var handleUnexpectedInfo = function(fault) {
  var header = createHeader(NAMESPACE_CONTROL,ERROR_UNEXPECTED_INFO);
  var payload = {
    "faultingParameter" : fault
  };
  return createEvent(header,payload);
}// handleUnexpectedInfo


// support functions
var createMessageId = function() {
  var d = new Date().getTime();
  var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    var r = (d + Math.random()*16)%16 | 0;
    d = Math.floor(d/16);
    return (c=='x' ? r : (r&0x3|0x8)).toString(16);
  });
  return uuid;
}// createMessageId


var createHeader = function(namespace, name, correlationToken) {
  var header = {
    "messageId": createMessageId(),
    "namespace": namespace,
    "name": name,
    "payloadVersion": "3",
    "correlationToken": correlationToken
  };
  return header;
}// createHeader

var createContext = function(namespace, name, value) {
  var ts = new Date();
  var properties = [ 
    {
      "namespace": namespace,
      "name": name,
      "value": value,
      "timeOfSample": ts.toISOString(),
      "uncertaintyInMilliseconds": 500
    }
  ];
  return {
    "context": { 
        "properties": properties
      }
  };
}// createHeader


var createEvent = function(header, payload, endpoint) {
  return {
    "event" : {
        "header" : header,
        "endpoint" : endpoint,
        "payload" : payload
    }
  };
}// createEvent

var createDirective = function(header, payload) {
  return {
    "directive" : {
        "header" : header,
        "payload" : payload
    }
  };

}// createDirective


var createEndpoint = function(endpointId) {
  return {
    "endpointId": endpointId,
    "scope": {
      "type": "BearerToken",
      "token": "access-token-from-Amazon"
    }
  };
}// createEndpoint




var pushState = function(desiredState) {
    var client = mqtt.connect('mqtt://'+MQTT_KEY+':'+MQTT_SECRET+'@'+MQTT_BROKER, {
      clientId: 'Alexa'
    });
    client.on('connect', function() {
      client.publish('/'+DEVICE_ID+'/desired', desiredState);
      client.end();
    });
}// pushstate



var log = function(title, msg) {
  console.log('**** ' + title + ': ' + JSON.stringify(msg));
}// log