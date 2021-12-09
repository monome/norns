//
// Created by emb on 11/28/18.
//

/*
 * jack client abstraction
 *
 * notice that inputs and outputs are assumed to exist in stereo pairs. 
 * the abstraction of a stereo pair of input ports is called a _sink_.
 * ... of output ports, a _source_.
 */

#ifndef CRONE_CLIENT_H
#define CRONE_CLIENT_H


#include <array>
#include <iostream>
#include <string>

#include <jack/jack.h>
#include <sstream>

#include "Commands.h"

namespace crone {

    template<int NumIns, int NumOuts>
    class Client {

    private:
        static_assert(NumIns % 2 == 0, "non-even input count");
        static_assert(NumOuts % 2 == 0, "non-even output count");

        typedef const jack_default_audio_sample_t* Source[2];
        typedef jack_default_audio_sample_t* Sink[2];

        std::array<jack_port_t*, NumIns> inPort;
        std::array<jack_port_t*, NumOuts> outPort;
        const char *name;

    protected:
        jack_client_t *client{};
        std::array<Source, NumIns/2> source;
        std::array<Sink, NumOuts/2> sink;

    private:
        // set up pointers for the current buffer
        void preProcess(jack_nframes_t numFrames) {
            int j=0;
            // FIXME: unroll with template?
            for(int i=0; i<NumIns/2; ++i) {
                source[i][0] = static_cast<const float*>(jack_port_get_buffer(inPort[j++], numFrames));
                source[i][1] = static_cast<const float*>(jack_port_get_buffer(inPort[j++], numFrames));
            }
            j = 0;
            for(int i=0; i<NumOuts/2; ++i) {
                sink[i][0] = static_cast<float*>(jack_port_get_buffer(outPort[j++], numFrames));
                sink[i][1] = static_cast<float*>(jack_port_get_buffer(outPort[j++], numFrames));
            }
        }
        // process using our source and sink pointers.
        // subclasses must implement this!
        virtual void process(jack_nframes_t numFrames) = 0;

        virtual void setSampleRate(jack_nframes_t sr) = 0;
    public:
        virtual void handleCommand(Commands::CommandPacket *p)  = 0;

    private:
        //---------------------------------
        //--- static handlers for jack API

        static int callback(jack_nframes_t numFrames, void*data) {
            auto *self = (Client*)(data);
            self->preProcess(numFrames);
            self->process(numFrames);
            return 0;
        }
        // static handler for shutdown from jack
        static void jack_shutdown(void* data) {
            (void)data;
            // FIXME: nothing to do?
        }

    public:
        virtual void setup() {
            using std::cerr;
            using std::cout;
            using std::endl;
            jack_status_t status;
            client = jack_client_open(name, JackNullOption, &status, nullptr);
            if(client == nullptr) {
                std::cerr << "jack_client_open() failed; status = " << status << endl;
                if (status & JackServerFailed) {
                    cerr << "unable to connect to JACK server" << endl;
                }
                throw;
            }
            if (status & JackServerStarted) {
                fprintf (stderr, "JACK server started\n");
            }
            if (status & JackNameNotUnique) {
                name = jack_get_client_name(client);
                fprintf (stderr, "unique name `%s' assigned\n", name);
            }

            jack_set_process_callback (client, Client::callback, this);
            jack_on_shutdown (client, jack_shutdown, this);


            auto sr = jack_get_sample_rate (client);
            std::cout << "engine sample rate: " <<  sr << std::endl;
            this->setSampleRate( sr );


            for(int i=0; i<NumIns; ++i) {
                std::ostringstream os;
                os << "input_" << (i+1);
                const char* name = os.str().c_str();
                inPort[i] = jack_port_register (client, name,
                                                JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                if(inPort[i] == nullptr) {
                    throw std::runtime_error("no inputs available");
                }
            }

            for(int i=0; i<NumOuts; ++i) {
                std::ostringstream os;
                os << "output_" << (i+1);
                const char* name = os.str().c_str();
                outPort[i] = jack_port_register (client, name,
                                                 JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                if(outPort[i] == nullptr) {
                    os << " failed to connect";
                    throw std::runtime_error(os.str());
                }
            }
        }

        void cleanup() {
            jack_client_close(client);
        }

    public:

        // NB: alas, we can't call setup() from c-tor because it includes a virtual method call
        // setup() will have to be called explicitly after creation
        explicit Client(const char* n) : name(n), client(nullptr) {
            std::cout << "constructed Client: " << name << std::endl;
        }
        virtual ~Client() = default;

        void start() {
            if (jack_activate (client)) {
                throw std::runtime_error("client failed to activate");
            }
        }

        void stop() {
            jack_deactivate(client);
        }

        void connectAdcPorts() {
            const char **ports = jack_get_ports (client, nullptr, nullptr,
                                                 JackPortIsPhysical|JackPortIsOutput);

            if (ports == nullptr) {
                throw std::runtime_error("no ADC ports found");
            }

            for(int i=0; i<NumIns; ++i) {
                if(i > 1) { break; }
                if (jack_connect (client, ports[i], jack_port_name (inPort[i]))) {

                    std::cerr << "failed to connect input port " << i << std::endl;
                    throw std::runtime_error("connectAdcPorts() failed");
                }
            }
            free(ports);
        }

        void connectDacPorts() {
            const char **ports = jack_get_ports (client, nullptr, nullptr,
                                                 JackPortIsPhysical|JackPortIsInput);

            if (ports == nullptr) {
                throw std::runtime_error("no DAC ports found");
            }

            for(int i=0; i<NumOuts; ++i) {
                if(i > 1) { break; }
                if (jack_connect (client, jack_port_name (outPort[i]), ports[i])) {
                    std::cerr << "failed to connect output port " << i << std::endl;
                    throw std::runtime_error("failed to connect output port");
                }
            }
            free(ports);
        }


        //---- getters
        const char* getInputPortName(int idx) {
            return jack_port_name(inPort[idx]);
        }

        const char* getOutputPortName(int idx) {
            return jack_port_name(outPort[idx]);
        }

        int getNumSinks() { return NumIns/2; }
        int getNumSources() { return NumOuts/2; }


        // FIXME: surely there is a cleaner way to use templated class reference parameter, here
        template<int N, int M>
        bool connect(Client<N,M> *other,
                     int sinkIdx, int sourceIdx) {
            if (sinkIdx >= this->getNumSinks()) {
                std::cerr << "invalid sink index in Client::connect()" << std::endl;
                return false;
            }
            if (sourceIdx >= other->getNumSources()) {
                std::cerr << "invalid source index in Client::connect()" << std::endl;
                return false;
            }
            for (int i=0; i<2; ++i) {
                const int srcPortIdx = sinkIdx*2 + i;
                const int dstPortIdx = sourceIdx*2 + i;
                const char* srcPortName = this->getOutputPortName(srcPortIdx);
                const char* dstPortName = other->getInputPortName(dstPortIdx);
                if(jack_connect(this->client, srcPortName, dstPortName)) {
                    std::cerr << "Client::connect(): port failed for channel " << i << std::endl;
                    return false;
                }
            }
            return true;
        }
    };

}

#endif //CRONE_CLIENT_H
