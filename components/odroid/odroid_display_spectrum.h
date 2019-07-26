#ifndef ODROID_DISPLAY_EMU_IMPL
void ili9341_write_frame_rectangle2(uint16_t* buffer, int size);
#else

void ili9341_write_frame_rectangle2(uint16_t* buffer, int size)
{
    int i=size;
    
    //xTaskToNotify = xTaskGetCurrentTaskHandle();
    while(i>1024) {
      uint16_t* line_buffer = line_buffer_get();
      memcpy(line_buffer, buffer, 1024 * 2);
      send_continue_line(line_buffer, 1024, 1);
      i-=1024;
      // buffer+=1024;
    }
    uint16_t* line_buffer = line_buffer_get();
    memcpy(line_buffer, buffer, i * 2);
    send_continue_line(line_buffer, i, 1);
}
#endif
