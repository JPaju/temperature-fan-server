# TempServer

## Usage

### List all temperatures

**Definition**

`GET /temperatures`

**Response**

- `200 OK`
```json
{
    "data": {
        "tempsensors": [
            {
                "id": "286D68D500000000",
                "temperature": 29
            },
			{
                "id": "286D68D600000000",
                "temperature": 37
            }
        ]
    }
}
```

### Search for new temperature sensors

**Definiton**

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
{
    "data": {
        "fans": [
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
    }
}
```

### List single fan

**Definition**

`GET /fans?pin=<pin>`

**Response**

- `200 OK` on success
```json
{
    "data": {
        "fan": {
            "pin": 3,
            "frequency": 25000,
            "dutycycle": 15
        }
    }
}
```
- `400 Bad request` on failure
```json
{
    "error": "Missing or invalid Parameters"
}
```

### List free fan pins

**Definition**

`GET /fans/freepins`

**Response**

- `200 OK`
```json
{
    "data": {
        "freepins": [
            9,
            10
        ]
    }
}
```

### List default values

**Definition**

`GET /fans/defaults`

**Response**

- `200 OK`
```json
{
    "data": {
        "defaults": {
            "dutycycle": 15,
            "frecuency": 25000,
            "min dutycycle": 15,
            "min frequency": 20,
            "max frequency": 32767
        }
    }
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
    "data": [
        {
            "pin": 3,
            "frequency": 25000,
            "dutycycle": 15
        }
    ]
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

`PUT /fans?pin=<pin>&dutycycle=<dutycycle>`

**Response**

- `204 No Content` on success
- `400 Bad request` on failure
```json
{
    "error": "Missing or invalid Parameters"
}
```

### Change fan's Frequency

**Definition**

`PUT /fans?pin=<pin>&frequency=<frequency>`

**Response**

- `204 No Content` on success
- `400 Bad request` on failure
```json
{
    "error": "Missing or invalid Parameters"
}
```
