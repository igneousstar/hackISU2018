/**
 * used to avoid a front edge
 */
void avoidFrontEdge(){
  setStop();
  setBackward();
  delay(500);
  setLeft();
  delay(1000);
  setStop();
}

/**
 * used to avoid right edge
 */
void avoidRightEdge(){
  setStop();
  setBackward();
  delay(500);
  setLeft();
  delay(500);
  setStop();
}

void avoidLeftEdge(){
  setStop();
  setBackward();
  delay(500);
  setRight();
  delay(500);
  setStop();
}

void avoidBump(){
  setStop();
  setBackward();
  delay(500);
  setLeft();
  delay(1000);
  setStop();
}

/**
 * gradually moves the bot forward
 * stops to let the sensors read
 * without noise from motors
 */
void moveForward(){
  setForward();
  delay(10);
  setStop();
  delay(2);
}
