
# class PublishQueuePosix 

Class for asynchronous publishing of events.

## Members

---

### PublishQueuePosix & PublishQueuePosix::withRamQueueSize(size_t size) 

Sets the RAM based queue size (default is 2)

```
PublishQueuePosix & withRamQueueSize(size_t size)
```

#### Parameters
* `size` The size to set (can be 0, default is 2)

You can set this to 0 and the events will be stored on the flash file system immediately. This is the best option if the events must not be lost in the event of a sudden reboot.

It's more efficient to have a small RAM-based queue and it eliminates flash wear. Make sure you set the size larger than the maximum number of events you plan to send out in bursts, as if you exceed the RAM queue size, all outstanding events will be moved to files.

---

### size_t PublishQueuePosix::getRamQueueSize() const 

Gets the size of the RAM queue.

```
size_t getRamQueueSize() const
```

---

### PublishQueuePosix & PublishQueuePosix::withFileQueueSize(size_t size) 

Sets the file-based queue size (default is 100)

```
PublishQueuePosix & withFileQueueSize(size_t size)
```

#### Parameters
* `size` The maximum number of files to store (one event per file)

If you exceed this number of events, the oldest event is discarded.

---

### size_t PublishQueuePosix::getFileQueueSize() const 

Gets the file queue size.

```
size_t getFileQueueSize() const
```

---

### PublishQueuePosix & PublishQueuePosix::withDirPath(const char * dirPath) 

Sets the directory to use as the queue directory. This is required!

```
PublishQueuePosix & withDirPath(const char * dirPath)
```

#### Parameters
* `dirPath` the pathname, Unix-style with / as the directory separator.

Typically you create your queue either at the top level ("/myqueue") or in /usr ("/usr/myqueue"). The directory will be created if necessary, however only one level of directory will be created. The parent must already exist.

The dirPath can end with a slash or not, but if you include it, it will be removed.

You must call this as you cannot use the root directory as a queue!

---

### const char * PublishQueuePosix::getDirPath() const 

Gets the directory path set using withDirPath()

```
const char * getDirPath() const
```

The returned path will not end with a slash.

---

### void PublishQueuePosix::setup() 

You must call this from setup() to initialize this library.

```
void setup()
```

---

### void PublishQueuePosix::loop() 

You must call the loop method from the global loop() function!

```
void loop()
```

---

### bool PublishQueuePosix::publish(const char * eventName, PublishFlags flags1, PublishFlags flags2) 

Overload for publishing an event.

```
bool publish(const char * eventName, PublishFlags flags1, PublishFlags flags2)
```

#### Parameters
* `eventName` The name of the event (63 character maximum).

* `flags1` Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.

* `flags2` (optional) You can use NO_ACK or WITH_ACK if desired.

#### Returns
true if the event was queued or false if it was not.

This function almost always returns true. If you queue more events than fit in the buffer the oldest (sometimes second oldest) is discarded.

---

### bool PublishQueuePosix::publish(const char * eventName, const char * data, PublishFlags flags1, PublishFlags flags2) 

Overload for publishing an event.

```
bool publish(const char * eventName, const char * data, PublishFlags flags1, PublishFlags flags2)
```

#### Parameters
* `eventName` The name of the event (63 character maximum).

* `data` The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).

* `flags1` Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.

* `flags2` (optional) You can use NO_ACK or WITH_ACK if desired.

#### Returns
true if the event was queued or false if it was not.

This function almost always returns true. If you queue more events than fit in the buffer the oldest (sometimes second oldest) is discarded.

---

### bool PublishQueuePosix::publish(const char * eventName, const char * data, int ttl, PublishFlags flags1, PublishFlags flags2) 

Overload for publishing an event.

```
bool publish(const char * eventName, const char * data, int ttl, PublishFlags flags1, PublishFlags flags2)
```

#### Parameters
* `eventName` The name of the event (63 character maximum).

* `data` The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).

* `ttl` The time-to-live value. If not specified in one of the other overloads, the value 60 is used. However, the ttl is ignored by the cloud, so it doesn't matter what you set it to. Essentially all events are discarded immediately if not subscribed to so they essentially have a ttl of 0.

* `flags1` Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.

* `flags2` (optional) You can use NO_ACK or WITH_ACK if desired.

#### Returns
true if the event was queued or false if it was not.

This function almost always returns true. If you queue more events than fit in the buffer the oldest (sometimes second oldest) is discarded.

---

### bool PublishQueuePosix::publishCommon(const char * eventName, const char * data, int ttl, PublishFlags flags1, PublishFlags flags2) 

Common publish function. All other overloads lead here. This is a pure virtual function, implemented in subclasses.

```
virtual bool publishCommon(const char * eventName, const char * data, int ttl, PublishFlags flags1, PublishFlags flags2)
```

#### Parameters
* `eventName` The name of the event (63 character maximum).

* `data` The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).

* `ttl` The time-to-live value. If not specified in one of the other overloads, the value 60 is used. However, the ttl is ignored by the cloud, so it doesn't matter what you set it to. Essentially all events are discarded immediately if not subscribed to so they essentially have a ttl of 0.

* `flags1` Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.

* `flags2` (optional) You can use NO_ACK or WITH_ACK if desired.

#### Returns
true if the event was queued or false if it was not.

This function almost always returns true. If you queue more events than fit in the buffer the oldest (sometimes second oldest) is discarded.

---

### void PublishQueuePosix::writeQueueToFiles() 

If there are events in the RAM queue, write them to files in the flash file system.

```
void writeQueueToFiles()
```

---

### void PublishQueuePosix::clearQueues() 

Empty both the RAM and file based queues. Any queued events are discarded.

```
void clearQueues()
```

---

### void PublishQueuePosix::setPausePublishing(bool value) 

Pause or resume publishing events.

```
void setPausePublishing(bool value)
```

#### Parameters
* `value` The value to set, true = pause, false = normal operation

If called while a publish is in progress, that publish will still proceed, but the next event (if any) will not be attempted.

This is used by the automated test tool; you probably won't need to manually manage this under normal circumstances.

---

### bool PublishQueuePosix::getPausePublishing() const 

Gets the state of the pause publishing flag.

```
bool getPausePublishing() const
```

---

### size_t PublishQueuePosix::getNumEvents() 

Gets the total number of events queued.

```
size_t getNumEvents()
```

This is the number of events in the RAM-based queue and the file-based queue. This operation is fast; the file queue length is stored in RAM, so this command does not need to access the file system.

If an event is currently being sent, the result includes this event.

---

### void PublishQueuePosix::lock() 

Lock the queue protection mutex.

```
void lock()
```

This is done internally; you probably won't need to call this yourself. It needs to be public for the WITH_LOCK() macro to work properly.

---

### bool PublishQueuePosix::tryLock() 

Attempt the queue protection mutex.

```
bool tryLock()
```

---

### void PublishQueuePosix::unlock() 

Unlock the queue protection mutex.

```
void unlock()
```

# struct PublishQueueEvent 

Structure to hold an event in RAM or in files.

In RAM, this structure is stored in the ramQueue.

On the flash file system, each file contains one event and consists of the PublishQueueFileHeader above (8 bytes) plus this structure.

Note that the eventData is specified as 1 byte here, but it's actually sized to fit the event data with a null terminator.

## Members

---

### PublishFlags flags 

NO_ACK or WITH_ACK. Can use PRIVATE, but that's no longer needed.

```
PublishFlags flags
```

---

### char eventName 

c-string event name (required)

```
char eventName
```

---

### char eventData 

Variable size event data.

```
char eventData
```

# struct PublishQueueFileHeader 

Structure stored before the event data in files on the flash file system.

Each file is sequentially numbered and has one event. The contents of the file are this header (8 bytes) followed by the PublishQueueEvent structure, which is variably sized based on the size of the event. <br/>

## Members

---

### uint32_t magic 

PublishQueuePosix::FILE_MAGIC = 0x31b67663.

```
uint32_t magic
```

---

### uint8_t version 

PublishQueuePosix::FILE_VERSION = 1.

```
uint8_t version
```

---

### uint8_t headerSize 

sizeof(PublishQueueFileHeader) = 8

```
uint8_t headerSize
```

---

### uint16_t nameLen 

sizeof(PublishQueueEvent::eventName) = 64

```
uint16_t nameLen
```

Generated by [Moxygen](https://sourcey.com/moxygen)