// EXAMPLE USAGE
SerialLogHandler logHandler;

void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop()
{
  // read from port 0, send to port 1:
  if (Serial.available())
  {
    Log.info("Serial");
    int inByte = Serial.read();
    Serial1.write(inByte);
  }
  // read from port 1, send to port 0:
  if (Serial1.available())
  {
    Log.info("Serial1");
    int inByte = Serial1.read();
    Serial.write(inByte);
  }
}