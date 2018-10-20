/*
 * bareblinkOS.cpp
 *
 * An example that runs directly on the blinkOS without blinklib
 *
 */


#include "blinkos.h"


void setupEntry() {

    // Blank

};

#define COOKIE_BYTE 0b10110111             // Non repeating bit pattern to detect errors. 

uint8_t cookie_byte_buffer = COOKIE_BYTE;         // Must be allocated in RAM so we can pass a pointer to it to senddata()

#define TX_BLIND_SEND_TIME_MS      250     // How often to do a blind send when no RX has happened recently to trigger ping pong

#define MISSED_RX_SHOW_TIME_MS     500     // How long to show blue when a RX timeout happens
#define RX_TIMEOUT_RX              200     // If we do not see a message in this long, then show a timeout

// Note these all init to 0's, which is what we want. 

struct face_t {
    
    millis_t next_tx_ms;          // Next time to transmit
    millis_t last_rx_ms;          // Last time a good packet seen
    
    uint8_t errorFlag;
};    

face_t faces[IR_FACE_COUNT];

void loopEntry( loopstate_in_t const *loopstate_in , loopstate_out_t *loopstate_out) {

    millis_t now_ms = loopstate_in->millis;
    
    // Clear out any errors on button press

    if ( loopstate_in->buttonstate.bitflags & BUTTON_BITFLAG_PRESSED ) {
        
        for( uint8_t f=0; f< IR_FACE_COUNT; f++ ) {
            
            if ( faces[f].errorFlag ) {
                                
                faces[f].errorFlag=0;
                
            }
            
        }   
        
    }  
    
    
    for( uint8_t f=0; f< IR_FACE_COUNT; f++ ){
        
        // Look for received packet....

        if ( loopstate_in->ir_data_buffers[f].ready_flag ) {

            if ( (loopstate_in->ir_data_buffers[f].len == 1 ) &&  loopstate_in->ir_data_buffers[f].data[0] == COOKIE_BYTE ) {
                    
                // Got a good packet!
                    
                faces[f].last_rx_ms = now_ms;
                    
                faces[f].next_tx_ms = now_ms;      // pong
                    
            } else {                    
                
                // Either len or cookie was wrong, so data transmission error
                
                faces[f].errorFlag = 1;
                
            }
            
        }                            
        
        // Update shown color
        
        if (faces[f].errorFlag) {
            
            // Red on error
            loopstate_out->colors[f] = pixelColor_t( 20 , 0, 0 , 1 );
            
        } else {
            
            millis_t elapsed_ms = now_ms - faces[f].last_rx_ms;     // Elapsed time since last RX            
            
            if (elapsed_ms < 200) {
                
                // Less than 200ms since last good RX, so show green
                loopstate_out->colors[f] = pixelColor_t( 0 , 20, 0 , 1 );
                
            } else if ( elapsed_ms < 500 ) {

                // More then 200ms since last good RX, so show red for 300ms
                loopstate_out->colors[f] = pixelColor_t( 0 , 0, 20 , 1 );

                
            } else {
                
                // Been long enough that we assume no one is there
                loopstate_out->colors[f] = pixelColor_t( 0 , 0, 0 , 1 );
                
            }                
            
        }            
        
        
        if ( faces[f].next_tx_ms < now_ms ) {
            
            // Time to send on this face
            
            ir_send_userdata( f , &cookie_byte_buffer , 1 );
            
            // Schedule a blind send in case we don't see a ping soon
            faces[f].next_tx_ms = now_ms + TX_BLIND_SEND_TIME_MS;
        
        }    // for(f)    
                    
    }    
    
}    