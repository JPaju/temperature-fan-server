# TempServer

## Usage

### List all temperatures

**Definition**

`GET /temperatures`

**Response**

- `200 OK`
```json
[
  {
    "id": "286D68D500000000",
    "temperature": 29
  },
  {
    "id": "286D68D600000000",
    "temperature": 37
  }
]
```

### Search for new temperature sensors

**Definition**

`PUT /temperatures/updatesensors`

**Response**

- `204 No Content`

### Request temperature update from all temperature sensors

**Definition**

`PUT /temperatures/updatetemps`

**Response**

- `204 No Content`


# FanServer

Fan speed (dutycycle) range from 0 to 100%. Dutycycle frequency is in Hz.

## Usage

### List all active fans

**Definition**

`GET /fans`

**Response**

- `200 OK`
```json
[
  {
    "pin": 3,
    "frequency": 25000,
    "dutycycle": 15
  },
  {
    "pin": 9,
    "frequency": 13000,
    "dutycycle": 80
  }
]
```

### List single fan

**Definition**

`GET /fans?pin=<pin>`

**Response**

- `200 OK` on success
```json
{
  "pin": 3,
  "frequency": 25000,
  "dutycycle": 15
}
```
- `400 Bad request` on failure
```json
{
  "error": "Missing or invalid Parameters"
}
```

### List config

**Definition**

`GET /fans/config`

**Response**

- `200 OK`
```json
{
  "defaults": {
    "dutycycle": 15,
    "frecuency": 25000,
  },
  "limits": {
    "min dutycycle": 15,
    "min frequency": 20,
    "max frequency": 32767  
  },
  "fanpins": [3, 9, 10]
}
```

### Add new fans

**Definition**

`POST /fans?pin=<pin>&frequency=<frequency>&dutycycle=<dutycycle>`

Only pin number is required, frequency and dutycycle are optional parameters. If no dutycycle or frequency is specified, default values will be used.

**Response**

- `201 Created` on success
```json
{
  "pin": 3,
  "frequency": 25000,
  "dutycycle": 15
}
```
- `400 Bad request` on failure
```json
{
  "error": "Missing or invalid Parameters"
}
```

### Delete existing fan

**Definition**

`DELETE /fans?pin=<pin>`

**Response**

- `204 No Content` on success
- `400 Bad request` on failure
```json
{
  "error": "Missing or invalid Parameters"
}
```

### Change fan's Dutycycle

**Definition**

`PUT /fans?pin=<pin>&dutycycle=<dutycycle>&frequency=<frequency>`

Not both dutycycle and frequency are required.

**Response**

- `200 OK` on success
```json
{
  "pin": 3,
  "frequency": 25000,
  "dutycycle": 15
}
```
- `400 Bad request` on failure
```json
{
  "error": "Missing or invalid Parameters"
}
```
