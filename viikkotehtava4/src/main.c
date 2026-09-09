//Tehtävän tekijät Salla-Mari Rokkonen ja Antti Ojala
//Tavoittelemme täyttä pistemäärää
//Tehtävässä on jatkettu edellisen tehtävän koodia, jonka vuoksi koodissa on ylimääräisiä toimintoja
//Tehtävä aloitettiin lisäämällä perussekvenssiin jokaiselle led taskille laskenta kauanko suoritus kestää. Mittauksista on otettu kuvakaappaukset, jotka palautetaan erikseen.
//Toiseksi tehtäväksi valittiin "Lisää debugtietoa" jossa ohjelmaan lisättiin Debug-taski ja tälle apufunktio.
//Tällä Debug-taskilla on pienempi prioriteetti kuin muilla taskeilla. Kaikki printk rivit korvattiin debug_printf riveillä, poislukien debug taskin sisällä oleva.
//Tämä lisäys laski sekvenssiin kuluvaa kokonaisaikaa huomattavasti, vaikka printtaukset oli käytössä (n.7000us->180us).
//Kolmanneksi tehtäväksi valittiin "Debugin asetus päälle / pois" joka toteutettiin lisäämällä debug enablointi lippu, jonka tilan debug_printf tarkistaa heti.
//Dispatcher taskiin lisättiin ehto kirjaimelle "D", jonka avulla avulla voidaan vaihtaa lipun tilaa terminaalissa.
 
 
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/timing/timing.h>
#include <stdarg.h>
// ========================================================
// 1. ASETUKSET JA MÄÄRITYKSET (Defines)
// ========================================================
// thread initialization
#define STACKSIZE 500
#define PRIORITY 5
 
// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
 
// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)
 
 
 
// ========================================================
// 2. TIETORAKENTEET (Structs)
// ========================================================
//FIFO-postipaketin sääntöjen luominen
struct data_t {
    void *fifo_reserved;
    char msg[20];
};
 
struct led_data_t {
    void *fifo_reserved;
    int time;
};
 
struct debug_data_t {
    void *fifo_reserved;
    char msg[80];
};
 
// ========================================================
// 3. LAITTEISTO (Ledit, Napit ja UART)
// ========================================================
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
 
// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
 
// #define BUTTON_0 DT_ALIAS(sw0)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;
//define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static struct gpio_callback button_1_data;
//define BUTTON_2 DT_ALIAS(sw2)
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static struct gpio_callback button_2_data;
//define BUTTON_3 DT_ALIAS(sw3)
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static struct gpio_callback button_3_data;
//define BUTTON_4 DT_ALIAS(sw4)
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});
static struct gpio_callback button_4_data;
 
 
// ========================================================
// 4. KÄYTTÖJÄRJESTELMÄN OBJEKTIT (FIFOt, Mutexit, Threadit)
// ========================================================
// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);
K_FIFO_DEFINE(red_fifo);
K_FIFO_DEFINE(yellow_fifo);
K_FIFO_DEFINE(green_fifo);
K_FIFO_DEFINE(debug_fifo);
 
K_MUTEX_DEFINE(led_mutex);
 
//ehtomuuttujat jokaiselle valolle
K_CONDVAR_DEFINE(red_condvar);
K_CONDVAR_DEFINE(yellow_condvar);
K_CONDVAR_DEFINE(green_condvar);
K_CONDVAR_DEFINE(release_condvar);
 
// Muistinvaraukset dynaamisille taskeille
K_THREAD_STACK_DEFINE(red_stack_area, STACKSIZE);
struct k_thread red_thread_data;
 
K_THREAD_STACK_DEFINE(yellow_stack_area, STACKSIZE);
struct k_thread yellow_thread_data;
 
K_THREAD_STACK_DEFINE(green_stack_area, STACKSIZE);
struct k_thread green_thread_data;
 
// Muuttuja, johon tallennetaan kaikkien valojen suoritusaikojen summa
uint64_t total_sequence_time_us = 0;
 
// Lippu debug-tulostukselle, jota debug_printf tarkistaa
int debug_enabled = 1; // 1 = päällä, 0 = pois päältä
 
// ========================================================
// 5. FUNKTIOIDEN ESITTELYT (Prototypes)
// ========================================================
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
 
void debug_task(void *, void *, void *);
void debug_printf(const char *format, ...);
 
int init_led(void);
int init_button(void);
int init_uart(void);
 
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
 
 
// ========================================================
// 6. PÄÄOHJELMA (Main)
// ========================================================
int main(void)
{
    timing_init();
    timing_start();
    timing_t start_time = timing_counter_get();
 
    init_led();
    init_uart();
 
    int ret = init_button();
    if (ret < 0) {
        return 0;
    }
 
    timing_t end_time = timing_counter_get();
    timing_stop();
    uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));
    uint64_t timing_us = timing_ns / 1000;
    debug_printf("Initialization: %lld\n", timing_us);
 
    while (1) {
        k_msleep(10); // sleep 10ms
    }
 
    return 0;
}
 
 
// ========================================================
// 7. ALUSTUSFUNKTIOT (Init)
// ========================================================
int init_uart(void) {
    //UART initialization = tarkistetaan, että sarjaportti on olemassa ja valmis
    if (!device_is_ready(uart_dev)) {
        debug_printf("UART is not ready\n");
        return 1;
    }
    return 0;
}
 
// Initialize leds
int  init_led() {
    // Led pin initialization
    int ret;
    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        debug_printf("Error: Led configure failed\n");        
        return ret;
    }
    // set led off
    gpio_pin_set_dt(&red,0);
 
    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        debug_printf("Error: Led configure failed\n");        
        return ret;
    }
    // set led off
    gpio_pin_set_dt(&green,0);
 
    debug_printf("Led initialized ok\n");
    return 0;
}
 
// Button initialization
int init_button() {
    int ret; // muuttuja kaikkien nappien virheentarkistukselle
 
    //button 0 Pause/resume
    if (!gpio_is_ready_dt(&button_0)) {
        debug_printf("Error: button 0 is not ready\n");
        return -1;
    }
    ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    if (ret != 0) {
        debug_printf("Error: failed to configure pin\n");
        return -1;
    }
    ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        debug_printf("Error: failed to configure interrupt on pin\n");
        return -1;
    }
    //Yhdistetään nappi 0 callback funktioon
    gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);
   
    //Button 1
    if (!gpio_is_ready_dt(&button_1)) {
        debug_printf("Error: button 1 is not ready\n");
        return -1;
    }
    ret = gpio_pin_configure_dt(&button_1, GPIO_INPUT);
    if (ret != 0) {
        debug_printf("Error: failed to configure pin\n");
        return -1;
    }
    ret = gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        debug_printf("Error: failed to configure interrupt on pin\n");
        return -1;
    }
    gpio_init_callback(&button_1_data, button_1_handler, BIT(button_1.pin));
    gpio_add_callback(button_1.port, &button_1_data);
 
    //Button 2
    if (!gpio_is_ready_dt(&button_2)) {
        debug_printf("Error: button 2 is not ready\n");
        return -1;
    }
    ret = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
    if (ret != 0) {
        debug_printf("Error: failed to configure pin\n");
        return -1;
    }
    ret = gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        debug_printf("Error: failed to configure interrupt on pin\n");
        return -1;
    }
    gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_data);
 
    //Button 3
    if (!gpio_is_ready_dt(&button_3)) {
        debug_printf("Error: button 3 is not ready\n");
        return -1;
    }
    ret = gpio_pin_configure_dt(&button_3, GPIO_INPUT);
    if (ret != 0) {
        debug_printf("Error: failed to configure pin\n");
        return -1;
    }
    ret = gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        debug_printf("Error: failed to configure interrupt on pin\n");
        return -1;
    }
    gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
    gpio_add_callback(button_3.port, &button_3_data);
 
    //Button 4
    if (!gpio_is_ready_dt(&button_4)) {
        debug_printf("Error: button 4 is not ready\n");
        return -1;
    }
    ret = gpio_pin_configure_dt(&button_4, GPIO_INPUT);
    if (ret != 0) {
        debug_printf("Error: failed to configure pin\n");
        return -1;
    }
    ret = gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        debug_printf("Error: failed to configure interrupt on pin\n");
        return -1;
    }
    gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
    gpio_add_callback(button_4.port, &button_4_data);
   
    return 0;
}
 
 
// ========================================================
// 8. KESKEYTYSPALVELUT (Interrupt Handlers)
// ========================================================
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    debug_printf("Button pressed\n");
}
 
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    debug_printf("Button 1 pressed\n");
}
 
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    debug_printf("Button 2 pressed (Yellow manual)\n");
}
 
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    debug_printf("Button 3 pressed (Green manual)\n");
}
 
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    debug_printf    ("Button 4 pressed (Yellow blink mode)\n");
}
 
 
// ========================================================
// 9. TASKIT (Threads)
// ========================================================
static void uart_task (void *unused1, void *unused2, void *unused3) {
    char rc=0; //Tähän tallennetaan yksi vastaanotettu kirjain kerrallaan
    char uart_msg[20]; // Tähän kerätään koko sana "RYG"
    memset(uart_msg,0,20); // tyhjennetään sana aluksi
    int uart_msg_cnt = 0; //laskuri, monesko kirjain on menossa
 
    while (true) {
        //uart_poll_in kysyy laitteelta tuliko uusi kirjain
        if (uart_poll_in(uart_dev, &rc) == 0) {
            debug_printf("Received: %c\n", rc);
            //Jos kirjain ei ole rivinvaihto (enter), lisätään se sanaan
            if (rc != '\r') {
                uart_msg[uart_msg_cnt] = rc;
                uart_msg_cnt++;
            }
            //Jos painettiin enter, sana on valmis lähetettäväksi
            else if (uart_msg_cnt > 0) {
                debug_printf("UART vastaanotti: %s\n", uart_msg);
               
                // 1. Luodaan uusi, tyhjä postipaketti (buf) muistiin
                struct data_t *buf = k_malloc(sizeof(struct data_t));
                if (buf != NULL) {
                    // 2. Kopioidaan kerätty sana pakettiin
                    strcpy(buf->msg, uart_msg);
                   
                    // 3. Laitetaan paketti työnjohtajan postilaatikkoon!
                    k_fifo_put(&dispatcher_fifo, buf);
                }
                // Tyhjennetään muisti seuraavaa sanaa varten
                uart_msg_cnt = 0;
                memset(uart_msg,0,20);
            }
        }
        k_msleep(10); // Nukutaan 10 millisekuntia
    }
}
 
static void dispatcher_task(void *unused1, void *unused2, void *unused3) {
    //Luodaan tallennus
    char history_colors[20];
    int history_times[20];
    int history_count = 0;
 
    while (true) {
        // dispatcher odottaa postia UARTilta (K_FOREVER = odottaa ikuisesti, jos postia ei ole)
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
       
        char sequence[20];
        strcpy(sequence, rec_item->msg); // Kopioidaan viesti talteen
        k_free(rec_item); // Heitetään tyhjä paketti roskiin, jotta muisti ei lopu
 
        debug_printf("Dispatcher: %s\n", sequence);
       
        char color = sequence[0]; // Ensimmäinen kirjain kertoo värin
        int time = atoi (sequence + 2); //Haetaan aika indeksistä 2
 
        //Tallennetaan väri ja aika listan ensimmäiseen vapaaseen paikkaan
        history_colors[history_count] = color;
        history_times[history_count] = time;
        history_count++;
 
        struct led_data_t *led_buf = k_malloc(sizeof(struct led_data_t));
        led_buf->time = time;
         
        if (color == 'R') {
            debug_printf("Dispatcher:punainen valo, aika: %d\n", time);
           
            k_fifo_put(&red_fifo, led_buf); //Lähetetään aika punaisen valon FIFOon
           
            k_mutex_lock(&led_mutex, K_FOREVER);
            //k_condvar_signal(&red_condvar);      // Herätetään punainen valo
            //Luodaan ja käynnistetään taski lennosta
            k_thread_create(&red_thread_data, red_stack_area, K_THREAD_STACK_SIZEOF(red_stack_area), red_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
            k_condvar_wait(&release_condvar, &led_mutex, K_FOREVER); // Odotetaan "olen valmis" -signaalia
            k_mutex_unlock(&led_mutex);          
           
        } else if (color == 'Y') {
            debug_printf("Dispatcher:keltainen valo, aika: %d\n", time);
           
            k_fifo_put(&yellow_fifo, led_buf); //Lähetetään aika keltaisen valon FIFOon
 
            k_mutex_lock(&led_mutex, K_FOREVER);
           // k_condvar_signal(&yellow_condvar);
            k_thread_create(&yellow_thread_data, yellow_stack_area, K_THREAD_STACK_SIZEOF(yellow_stack_area), yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
            k_condvar_wait(&release_condvar, &led_mutex, K_FOREVER);
            k_mutex_unlock(&led_mutex);
           
        } else if (color == 'G') {
            debug_printf("Dispatcher:vihreä valo, aika: %d\n", time);
           
            k_fifo_put(&green_fifo, led_buf); //Lähetetään aika vihreän valon FIFOon
 
            k_mutex_lock(&led_mutex, K_FOREVER);
            //k_condvar_signal(&green_condvar);
            k_thread_create(&green_thread_data, green_stack_area, K_THREAD_STACK_SIZEOF(green_stack_area), green_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
            k_condvar_wait(&release_condvar, &led_mutex, K_FOREVER);
            k_mutex_unlock(&led_mutex);
           
        } else if (color == 'T') {
            debug_printf("toistetaan %d käskyä\n", history_count);
 
            for (int i = 0; i < history_count; i++) {
                //Haetaan väri ja aika listasta
                char old_color = history_colors[i];
                int old_time = history_times[i];
 
                struct led_data_t *led_buf = k_malloc(sizeof(struct led_data_t));
                led_buf->time = old_time;
 
                if (old_color == 'R') {
                    k_fifo_put(&red_fifo, led_buf);
                    k_mutex_lock(&led_mutex, K_FOREVER);
                    k_thread_create(&red_thread_data, red_stack_area, K_THREAD_STACK_SIZEOF(red_stack_area), red_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
                    k_condvar_wait(&release_condvar, &led_mutex, K_FOREVER);
                    k_mutex_unlock(&led_mutex);
                } else if (old_color == 'Y') {
                    k_fifo_put(&yellow_fifo, led_buf);
                    k_mutex_lock(&led_mutex, K_FOREVER);
                    k_thread_create(&yellow_thread_data, yellow_stack_area, K_THREAD_STACK_SIZEOF(yellow_stack_area), yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
                    k_condvar_wait(&release_condvar, &led_mutex, K_FOREVER);
                    k_mutex_unlock(&led_mutex);
                } else if (old_color == 'G') {
                    k_fifo_put(&green_fifo, led_buf);
                    k_mutex_lock(&led_mutex, K_FOREVER);
                    k_thread_create(&green_thread_data, green_stack_area, K_THREAD_STACK_SIZEOF(green_stack_area), green_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
                    k_condvar_wait(&release_condvar, &led_mutex, K_FOREVER);
                    k_mutex_unlock(&led_mutex);
                }  
            }
        // Jos kirjain on D, vaihdetaan debug-lipun tilaa
        } else if (color == 'D') {
            debug_enabled = !debug_enabled;
            // Käytetään suoraan printk, jotta statusviesti näkyy aina riippumatta lipun tilasta
            printk("Debug prints: %s\n", debug_enabled ? "ON" : "OFF");
        }
       
        debug_printf("Kokonaisaika: %lld us\n", total_sequence_time_us);
        total_sequence_time_us = 0;
    }
}
// apufunktio debug_printf, joka tarkistaa lipun ja tulostaa viestin FIFOon
void debug_printf(const char *format, ...) {
 
    // Tarkistetaan, onko debug-tulostus päällä
    if (!debug_enabled) {
        return;
    }
    struct debug_data_t *dbuf = k_malloc(sizeof(struct debug_data_t));
    if (dbuf != NULL) {
        va_list args;
        va_start(args, format);
        // vsnprintf täyttää viestin turvallisesti annetuilla muuttujilla
        vsnprintf(dbuf->msg, sizeof(dbuf->msg), format, args);
        va_end(args);
       
        k_fifo_put(&debug_fifo, dbuf);
    }
}
 
void debug_task(void *, void *, void *) {
    while (true) {
        // Odotetaan uutta tulostettavaa viestiä
        struct debug_data_t *item = k_fifo_get(&debug_fifo, K_FOREVER);
       
        // Tulostetaan viesti suoraan
        printk("%s", item->msg);
       
        // Vapautetaan muisti
        k_free(item);
    }
}
 
// Task to handle red led
void red_led_task(void *, void *, void*) {
   
    debug_printf("Red led thread started\n");
 
        timing_start();
        timing_t red_start_time = timing_counter_get();
 
        //Haetaan paketti
        struct led_data_t *rec_item = k_fifo_get(&red_fifo, K_FOREVER);
        int sleep_time = rec_item->time; //Otetaan aika talteen
        k_free(rec_item); // heitetään tyhjä paketti pois, ettei muisti lopu
        //Valo päälle ja pois
        gpio_pin_set_dt(&red, 1);
        debug_printf("Red on for %d ms\n", sleep_time);
        k_msleep(sleep_time); // Nukutaan annettu aika
        gpio_pin_set_dt(&red, 0);
        debug_printf("Red off\n");
        timing_t red_end_time = timing_counter_get();
        timing_stop();
        uint64_t red_timing_ns = timing_cycles_to_ns(timing_cycles_get(&red_start_time, &red_end_time));
        uint64_t red_timing_us = red_timing_ns / 1000;
        debug_printf("Red led task execution time: %lld us\n", red_timing_us);
        total_sequence_time_us = total_sequence_time_us + red_timing_us;
        //Ilmoitetaan dispatcherille, että valo on valmis
        k_mutex_lock(&led_mutex, K_FOREVER);
        k_condvar_signal(&release_condvar);
        k_mutex_unlock(&led_mutex);
    //}
}
 
// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
   
    debug_printf("Yellow led thread started\n");
   
        timing_start();
        timing_t yellow_start_time = timing_counter_get();
 
        //Valo päälle ja pois
        struct led_data_t *rec_item = k_fifo_get(&yellow_fifo, K_FOREVER);
        int sleep_time = rec_item->time;
        k_free(rec_item);
       
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);
        debug_printf("Yellow on for %d ms\n", sleep_time);
        k_msleep(sleep_time); // Nukutaan annettu aika
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
        debug_printf("Yellow off\n");
        timing_t yellow_end_time = timing_counter_get();
        timing_stop();
        uint64_t yellow_timing_ns = timing_cycles_to_ns(timing_cycles_get(&yellow_start_time, &yellow_end_time));
        uint64_t yellow_timing_us = yellow_timing_ns / 1000;
        debug_printf("Yellow led task execution time: %lld us\n", yellow_timing_us);
        total_sequence_time_us = total_sequence_time_us + yellow_timing_us;
        //Ilmoitetaan dispatcherille, että valo on valmis
        k_mutex_lock(&led_mutex, K_FOREVER);
        k_condvar_signal(&release_condvar);
        k_mutex_unlock(&led_mutex);
    //}
}
 
// Task to handle green led
void green_led_task(void *, void *, void*) {
   
    debug_printf("Green led thread started\n");
 
        timing_start();
        timing_t green_start_time = timing_counter_get();
 
        //Valo päälle ja pois
        struct led_data_t *rec_item = k_fifo_get(&green_fifo, K_FOREVER);
        int sleep_time = rec_item->time;
        k_free(rec_item);
 
        gpio_pin_set_dt(&green, 1);
        debug_printf("Green on for %d ms\n", sleep_time);
        k_msleep(sleep_time); // Nukutaan annettu aika
        gpio_pin_set_dt(&green, 0);
        debug_printf("Green off\n");
        timing_t green_end_time = timing_counter_get();
        timing_stop();
        uint64_t green_timing_ns = timing_cycles_to_ns(timing_cycles_get(&green_start_time, &green_end_time));
        uint64_t green_timing_us = green_timing_ns / 1000;
        debug_printf("Green led task execution time: %lld us\n", green_timing_us);
        total_sequence_time_us = total_sequence_time_us + green_timing_us;
        //Ilmoitetaan dispatcherille, että valo on valmis
        k_mutex_lock(&led_mutex, K_FOREVER);
        k_condvar_signal(&release_condvar);
        k_mutex_unlock(&led_mutex);
    //}
}
 
 
// ========================================================
// 10. JATKUVAT TASKIT (Thread Defines)
// ========================================================
//Komento UART taskin automaattista käynnistystä varten
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
 
// Käsketään Zephyriä käynnistämään dispatcher taustalle
K_THREAD_DEFINE(dis_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
// pienenmpi prioriteetti debug taskille
K_THREAD_DEFINE(debug_thread, STACKSIZE, debug_task, NULL, NULL, NULL, 7, 0, 0);
 
 