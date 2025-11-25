#include "Alpha.h"
#include "XPLink.h"
#include "dmacirc.h"
#include "uart_rx.h"

uint8_t ALPHA_STATE_INIT(Alpha *a)
{
    a->going = 1;

    return 0;
}

uint8_t ALPHA_SENSORS_INIT(Alpha *a)
{
    // initialize TC
    a->tc1.spi_tx = HAL_SPI_Transmit;
    a->tc1.spi_rx = HAL_SPI_Receive;
    a->tc1.s = &hspi1;
    a->tc2.spi_tx = HAL_SPI_Transmit;
    a->tc2.spi_rx = HAL_SPI_Receive;
    a->tc2.s = &hspi1;
    a->tc3.spi_tx = HAL_SPI_Transmit;
    a->tc3.spi_rx = HAL_SPI_Receive;
    a->tc3.s = &hspi1;
    a->tc4.spi_tx = HAL_SPI_Transmit;
    a->tc4.spi_rx = HAL_SPI_Receive;
    a->tc4.s = &hspi1;

    max31856_init(&(a->tc1));
    max31856_init(&(a->tc2));
    max31856_init(&(a->tc3));
    max31856_init(&(a->tc4));
    // set thermocouple type
    max31856_write_register(&(a->tc1), 0x01, 0b00000011, 1);
    max31856_write_register(&(a->tc2), 0x01, 0b00000011, 2);
    max31856_write_register(&(a->tc3), 0x01, 0b00000011, 3);
    max31856_write_register(&(a->tc4), 0x01, 0b00000011, 4);
    // start conversion
    max31856_write_register(&(a->tc1), 0x00, 0b10010000, 1);
    max31856_write_register(&(a->tc2), 0x00, 0b10010000, 2);
    max31856_write_register(&(a->tc3), 0x00, 0b10010000, 3);
    max31856_write_register(&(a->tc4), 0x00, 0b10010000, 4);

    // initialize pressure sensors
    ads7828_init_external_ref(&(a->ads1), &hi2c1, 0x48, stm32_i2c_write, stm32_i2c_read, 5.0f);
    ads7828_init_external_ref(&(a->ads2), &hi2c1, 0x49, stm32_i2c_write, stm32_i2c_read, 5.0f);

    return 0;
}

uint8_t ALPHA_COMMS_INIT(Alpha *a)
{
    // initialize DMA
    dmasendinit(&huart1);

    // initialize uart rx
    uart_rx_init(&huart1);

    return 0;
}

uint8_t ALPHA_READ_TEMP(Alpha *a)
{

    // read and send temp data
    a->temp_1 = max31856_read_thermocouple(&(a->tc1), 1);
    a->temp_2 = max31856_read_thermocouple(&(a->tc2), 2);
    a->temp_3 = max31856_read_thermocouple(&(a->tc3), 3);
    a->temp_4 = max31856_read_thermocouple(&(a->tc4), 4);

    return 0;
}

uint8_t ALPHA_READ_PRESSURE(Alpha *a)
{

    a->p1 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_2_COM);
    a->p2 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_3_COM);
    a->p3 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_4_COM);
    a->p4 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_5_COM);
    a->p5 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_6_COM);
    a->p6 = ads7828_read_digit(&(a->ads1), ADS7828_CHANNEL_7_COM);

    a->p7 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_0_COM);
    a->p8 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_1_COM);
    a->p9 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_2_COM);
    a->p10 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_3_COM);
    a->p11 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_4_COM);
    a->p12 = ads7828_read_digit(&(a->ads2), ADS7828_CHANNEL_5_COM);

    return 0;
}

uint8_t ALPHA_SEND_10HZ(Alpha *a)
{
    // send all temp data
    xp_packet_t pkt1;
    pkt1.data = (uint64_t)(a->temp_1);
    pkt1.type = TEMP1;
    xp_packet_t pkt2;
    pkt2.data = (uint64_t)(a->temp_2);
    pkt2.type = TEMP2;
    xp_packet_t pkt3;
    pkt3.data = (uint64_t)(a->temp_3);
    pkt3.type = TEMP3;
    xp_packet_t pkt4;
    pkt4.data = (uint64_t)(a->temp_4);
    pkt4.type = TEMP4;

    uint8_t packet[12];

    XPLINK_PACK(packet, &pkt1);
    dmasend(packet, 12);
    XPLINK_PACK(packet, &pkt2);
    dmasend(packet, 12);
    XPLINK_PACK(packet, &pkt3);
    dmasend(packet, 12);
    XPLINK_PACK(packet, &pkt4);
    dmasend(packet, 12);

    return 0;
}

uint8_t Alpha_Send_100HZ(Alpha *a)
{
    uint16_t vals[] = {a->p1,
                       a->p2,
                       a->p3,
                       a->p4,
                       a->p5,
                       a->p6,
                       a->p7,
                       a->p8,
                       a->p9,
                       a->p10,
                       a->p11,
                       a->p12};

    for (int i = 0; i < 12; i++)
    {
        xp_packet_t pkt;
        pkt.data = vals[i];
        pkt.type = PRESSURE1 + i;

        uint8_t packet[12];

        XPLINK_PACK(packet, &pkt);
        dmasend(packet, 12);
    }

    return 0;
}

uint8_t ALPHA_RX(Alpha *a)
{
    RX_HANDLER(a);

    return 0;
}
uint8_t ALPHA_CHECK_SAFETY(Alpha *a)
{

    // Add safety checks here
    uint8_t safety_ok =1;
    return safety_ok;
}

uint8_t ALPHA_CONTROL_VALVES (Alpha *a,uint8_t valve_state)
{

    // Add valve state control logic/open and close logic 
}

uint8_t ALPHA_STATE_MACHINE(Alpha *a)
{
    //REad keyswitches
    uint8_t key1=HAL_GPIO_ReadPin(K1_GPIO_Port,K1_Pin);
    //Add other keyswitch???

    a->both_keys_pressed=(key1==GPIO_PIN_SET) && (key2=GPIO_PIN_SET);

    //check for safety conditions 
    uint8_t safety_ok = ALPHA_CHECK_SAFETY(a);

    // Check time limit 
    if (HAL_GetTick() -a->mission_start_time>MAX_MISSION_TIME_MS){

       a->time_limit_exceeded =1;

    }
    //Check Burn wire Status 
    a->burn_wire_cut= ALPHA_CHECK_BURN_WIRE(a);
     
    switch(a->current_state) {
        case STATE_TESTING_FILL:
            //Send sensor data
            //Check for inoming serial commands 
            if (a->both_keys_pressed && a->ignition_serial_recieved)
            {
                a->current_state=STATE_IGNTTION;
                a->igntition_start_time=HAL_GetTick();
                a->HIGH_RATE_LOGGING =1;
            
            }
            // Safety abort
            if (!safety_ok)
            {
                a->current_state=STATE_POWER_OFF;
            }
            break;
        case STATE_IGNTTION:
            //Begin high rate data logging 
            a-> high_rate_logging=1;

            //Add logic to fire pyro (Not sure how this links in with communication)

            if (a->burn_wire_cut)
            {
                a->current_state=STATE_BURN_WIRE_CUT;
                a->steady_start_time=HAL_GetTick();
            }
            if (!safety_ok)
            {
                a->current_state=STATE_POWER_OFF;
                HAL_GPIO_WritePin(POWER_GPIO_Port, POWER_Pin, GPIO_PIN_RESET);
            }
            break;
        }
        case STATE_BURN_WIRE_CUT:
            //Continue high rate data logging 
            if (a->time_limit_exceeded)
            {
                a->current_state=STATE_SHUTDOWN;
                a->shutdown_start_time=HAL_GetTick();
            } else {
                a->current_state=STATE_STEADY_STATE;
            /// THIS DOES NOT PROPERLY SHUTDOWN IN CASE OF FAILED IGNTION 
            }
            break;
        case STATE_STEADY_STATE:
             // TODO: Indicate steady state with LED
            // HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
            
            // TODO: Define what satisfies the shutdown timer
            // Example: run for certain duration in steady state
            if (HAL_GetTick()-a->steady_state_timer > SHUTDOWN_WAIT_TIME_MS)
            {
                a->shutdown_timer_satisfied = 1;
            }
            if (a->shutdown_timer_satisfied)
            {
                a->current_state=STATE_SHUTDOWN;
                a->shutdown_start_time=HAL_GetTick();
            }

            //safety abort 
            if (!safety_ok {
                a->current_state=STATE_POWER_OFF;
                
            }
            break;
        case STATE_SHUTDOWN:
            // Continue logging data
            //CLose all valves
            ALPHA_CONTROL_VALVES(a,0);

            // Disable pyro 
            HAL_GPIO_WritePin(POWER_GPIO_Port, POWER_Pin, GPIO_PIN_RESET);

            uint8_t wait_complete=0;
            if (HAL_GetTick()-a->sthudown_start_time > SHUTDOWN_WAIT_TIME_MS){
                wait_complete=1;
            }

            if (wait_complete){
                a->current_state=STATE_POWER_OFF;
            }
            break;
        case STATE_POWER_OFF:
             a->high_rate_logging=0;
             a->going=0;

             ALPHA_CONTROL_VALVES(a,0);
             HAL_GPIO_WritePin(PYRO1_GPIO_Port, PYRO1_Pin, GPIO_PIN_RESET);
             break;
    }       


    return 0;      
        )
}