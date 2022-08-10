#include <cstdio>
#include <sys/mman.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <thread>
#include <vector>
#include <cstring>

#define _BSD_SOURCE

#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define RADIO_PERIPH_ADDRESS 0x43c00000

#define STREAM_VALID_OFFSET 4
#define STREAM_DATA_OFFSET 5
#define STREAM_RESET_OFFSET 6
#define RADIO_STREAM_ADDRESS 0x43c00000

using namespace std;

static std::atomic<bool> _g_running(true);
static std::atomic<bool> _g_stream_en(true);

// the below code uses a device called /dev/mem to get a pointer to a physical
// address.  We will use this pointer to read/write the custom peripheral
volatile unsigned int * get_a_pointer(unsigned int phys_addr)
{

	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); 
	void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr); 
	volatile unsigned int *radio_base = (volatile unsigned int *)map_base; 
	return (radio_base);
}


void radioTuner_tuneRadio(volatile unsigned int *ptrToRadio, float tune_frequency)
{
	float pinc = (-1.0*tune_frequency)*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_TUNER_PINC_OFFSET)=(int)pinc;
}

void radioTuner_setAdcFreq(volatile unsigned int* ptrToRadio, float freq)
{
	float pinc = freq*(float)(1<<27)/125.0e6;
	*(ptrToRadio+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = (int)pinc;
}

void play_tune(volatile unsigned int *ptrToRadio, float base_frequency)
{
	int i;
	float freqs[16] = {1760.0,1567.98,1396.91, 1318.51, 1174.66, 1318.51, 1396.91, 1567.98, 1760.0, 0, 1760.0, 0, 1760.0, 1975.53, 2093.0,0};
	float durations[16] = {1,1,1,1,1,1,1,1,.5,0.0001,.5,0.0001,1,1,2,0.0001};
	for (i=0;i<16;i++)
	{
		radioTuner_setAdcFreq(ptrToRadio,freqs[i]+base_frequency);
		usleep((int)(durations[i]*500000));
	}
}


void print_benchmark(volatile unsigned int *periph_base)
{
    // the below code does a little benchmark, reading from the peripheral a bunch 
    // of times, and seeing how many clocks it takes.  You can use this information
    // to get an idea of how fast you can generally read from an axi-lite slave device
    unsigned int start_time;
    unsigned int stop_time;
    start_time = *(periph_base+RADIO_TUNER_TIMER_REG_OFFSET);
    for (int i=0;i<2048;i++)
        stop_time = *(periph_base+RADIO_TUNER_TIMER_REG_OFFSET);
    printf("Elapsed time in clocks = %u\n",stop_time-start_time);
    float throughput=0;
    // please insert your code here for calculate the actual throughput in Mbytes/second
    // how much data was transferred? How long did it take?
    throughput = (2048*4) / ((float)(stop_time - start_time) / 125e6) / 1e6;
    printf("Estimated Transfer throughput = %f Mbytes/sec\n",throughput);
}
void show_menu (void) {
    cout << "Radio CLI Menu" << endl;
    cout << "       u/U : increment frequency by 100/1000" << endl;
    cout << "       d/D : decrement frequency by 100/1000" << endl;
    cout << "     t <x> : set the tuner frequency to x" << endl;
    cout << "     f <x> : set the radio frequency to x" << endl;
    cout << "     s <x> : 's enable' to begin streaming, 's disable' to end streaming" << endl;
    cout << "         m : show the menu again" << endl;
    cout << "         e : exit" << endl;

}
int radio_cli(void) {
    volatile unsigned int *my_periph = get_a_pointer(RADIO_PERIPH_ADDRESS);	
    int freq = 0;
    int old_freq = 0;
    int tuner_freq = 0;
    int old_tuner_freq = 0;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    radioTuner_setAdcFreq(my_periph,freq);
    radioTuner_tuneRadio(my_periph,tuner_freq);
    show_menu();

    
    while (_g_running) {
        cout << "SDRCLI:" << flush;
        int ret = select(STDIN_FILENO+1, &fds, NULL, NULL, NULL);
        if (ret > 0) {
            string input_str;
            getline(cin, input_str);
            size_t idx = input_str.find_first_not_of(" \t\r\n");
            if (idx != string::npos) {
                string arg;
                //make argument lowercase
                std::transform(arg.begin(), arg.end(), arg.begin(), [](unsigned char c) {return std::tolower(c);});
                if (idx + 1 < input_str.size()) {
                    arg = input_str.substr(idx+1);
                }
                char cmd = input_str.at(idx);
                switch(cmd) {
                    case 'u': freq += 100; break;
                    case 'U': freq += 1000; break;
                    case 'd': freq -= 100; break;
                    case 'D': freq -= 1000; break;
                    case 't':
                    case 'T':
                        tuner_freq = stoi(arg); break;
                    case 'f':
                    case 'F':
                        freq = stoi(arg); break;
                    case 'm':
                    case 'M':
                        show_menu(); break;
                    case 's':
                    case 'S':
                        if (arg.find("enable") != string::npos) {
                            _g_stream_en = true;
                            cout << "UDP Streaming enabled" << endl;
                        } else if (arg.find("disable") != string::npos) {
                            _g_stream_en = false;
                            cout << "UDP Streaming disabled" << endl;
                        }
                        break;
                    case 'e':
                    case 'E':
                        _g_running = false; break;
                    default:
                        cout << "Invalid command" <<endl; break;
                }
            }
            if (old_freq != freq) {
                cout << "Radio Frequency set to " << freq << endl;
                radioTuner_setAdcFreq(my_periph,freq);
                old_freq = freq;
            }
            if (old_tuner_freq != tuner_freq) {
                cout << "Tuner Frequency set to " << tuner_freq << endl;
                radioTuner_tuneRadio(my_periph,tuner_freq);
                old_tuner_freq = tuner_freq;
            }
        }
    }
    cout << "SDRCLI Exiting" << endl;
    radioTuner_setAdcFreq(my_periph,0);
    radioTuner_tuneRadio(my_periph,0);
    return 0;
}
int radio_udp_bcaster(string ip_addr, string port) {
    volatile unsigned int *my_periph = get_a_pointer(RADIO_STREAM_ADDRESS);	
    sockaddr_in bcast_sock;
    int ret = inet_pton(AF_INET, ip_addr.c_str(), &(bcast_sock.sin_addr));
    //test IP address
    if (ret != 1) {
        cout << "Invalid IP Address" << endl;
        _g_running = false;
        return -1;
    }
    bcast_sock.sin_port = htons(stoi(port));
    int bcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bcast < 0) {
        cout << "Unable to open UDP socket" << endl;
        _g_running = false;
        return -1;
    }

    sockaddr_in sock;
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = INADDR_ANY;
    sock.sin_port = htons(stoi(port));
    ret = bind(bcast, (const struct sockaddr *) & sock, sizeof(sock));
    if (ret < 0) {
        cout << "Failed to bind UDP socket" << endl;
        _g_running = false;
        return -1;
    }
    

    uint16_t frame_count = 0;
    vector<uint8_t> packet;
    vector<uint32_t> data_vec;
    data_vec.resize(256);
    *(my_periph + STREAM_RESET_OFFSET) = 0x00000001;
    bool enabled = false;
    while (_g_running) {
        while (_g_stream_en && _g_running){
            if(!enabled) {
                *(my_periph + STREAM_RESET_OFFSET) = 0x00000000;
                enabled = true;
            }
            //copy out and broacast data
            size_t samples = 0;
            while (samples < 256 && _g_running && _g_stream_en) {
                uint32_t valid = *(my_periph + STREAM_VALID_OFFSET);
                if (valid) {
                    data_vec[samples] = *(my_periph + STREAM_DATA_OFFSET);
                    samples++;
                }
            }
            packet.resize(sizeof(frame_count) + sizeof(uint32_t) * data_vec.size());
            unsigned char* p_packet = packet.data();
            memcpy(p_packet, &frame_count, sizeof(frame_count));
            p_packet += sizeof(frame_count);
            memcpy(p_packet, (char*)data_vec.data(), data_vec.size() * sizeof(uint32_t));

            sendto(bcast, (const char *) packet.data(), packet.size(), MSG_CONFIRM, (const struct sockaddr*) &bcast_sock, sizeof(bcast_sock));
            frame_count++;
        }
        //don't hog the CPU busy waiting
        if (!_g_stream_en) {
            *(my_periph + STREAM_RESET_OFFSET) = 0x00000001;
            enabled = false;
        }
        usleep(200 * 1000);
        
    }
    return 0;
}

int main(int argc, char ** argv)
{
    vector<string> args;
    for (int i = 0; i < argc; ++i) {
        string arg(argv[i]);
        args.push_back(arg);
    }
    if (argc != 3) {
        cout << "Usage: sdr <ip_addr> <port>" << endl;
        return 0;
    }

    cout << "\r\n\r\n\r\nLab 6 James Saams - Custom Peripheral Demonstration" << endl;
    //launch udp broadcaster
    thread bcast_thd(radio_udp_bcaster, args[1], args[2]);
    radio_cli();
    bcast_thd.join();
    return 0;
}
