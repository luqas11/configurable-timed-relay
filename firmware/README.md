# Firmware

### How this works
This ESP8266 sketch connects the board to the WiFi network, and fetches the current time periodically using the [Arduino Time Library](https://github.com/PaulStoffregen/Time). When the configured time matches the current time, the board turns on the relay modules for the specified duration. The configuration values can be set from a webpage server at the board's IP, which exchanges information dynamically with it through exposed endpoints.

### API

`[GET] /get-values`  

Returns the current configured values, with the time in minutes and the duration in seconds. For example:
```
{
    "time": 570,
    "duration": 30
}
```

`[GET] /set-values?time={time}&duration={duration}`  

Sets the configures values, with the time in minutes and the duration in seconds. The given name can only contain numbers, and be lesser than some defined maximums. For example, the query params could be:
```
/set-values?time=1048&duration=25
```

`[GET] /get-current-hour`  

Returns the current system hour in minutes. For example:
```
{
    "currentHour": "1035"
}
```
