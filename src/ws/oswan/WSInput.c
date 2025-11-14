#include "WSInput.h"

int ws_input_poll(int mode);  // implement this

int WsInputGetState(int mode)
{
  return ws_input_poll(mode);
}
