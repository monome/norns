//
// Created by emb on 11/18/18.
//


#include <cstdio>
#include <cstdlib>

#include "AudioMain.h"
#include "JackClient.h"
#include <jack/jack.h>

using namespace crone;

class JackClient::Imp {

    friend class JackClient;
private:
    AudioMain audioMain;
    jack_port_t *input_port[4];
    jack_port_t *output_port[2];
    jack_client_t *client;

    static const jack_default_audio_sample_t *in_adc[2];
    static const jack_default_audio_sample_t *in_ext[2];
    static jack_default_audio_sample_t *out[2];

    static int process (jack_nframes_t numFrames, void *data) {

        auto *imp = (JackClient::Imp*)(data);

        in_adc[0] = (jack_default_audio_sample_t *) jack_port_get_buffer (imp->input_port[0], numFrames);
        in_adc[1] = (jack_default_audio_sample_t *) jack_port_get_buffer (imp->input_port[1], numFrames);
        in_ext[0] = (jack_default_audio_sample_t *) jack_port_get_buffer (imp->input_port[2], numFrames);
        in_ext[1] = (jack_default_audio_sample_t *) jack_port_get_buffer (imp->input_port[3], numFrames);
        out[0] = (jack_default_audio_sample_t *) jack_port_get_buffer (imp->output_port[0], numFrames);
        out[1] = (jack_default_audio_sample_t *) jack_port_get_buffer (imp->output_port[1], numFrames);

        imp->audioMain.processBlock(in_adc, in_ext, out, numFrames);

        return 0;
    }

    static void jack_shutdown(void* data) {
        (void)data;
        // nothing to do...
        // but here we would handle forced shutdown from jack server
    }


public:
    void setup() {
        const char *client_name = "crone";
        const char *server_name = nullptr;
        jack_options_t options = JackNullOption;
        jack_status_t status;

        /* open a client connection to the JACK server */

        client = jack_client_open (client_name, options, &status, server_name);
        if (client == nullptr) {
            fprintf (stderr, "jack_client_open() failed, "
                             "status = 0x%2.0x\n", status);
            if (status & JackServerFailed) {
                fprintf (stderr, "Unable to connect to JACK server\n");
            }
            exit (1);
        }
        if (status & JackServerStarted) {
            fprintf (stderr, "JACK server started\n");
        }
        if (status & JackNameNotUnique) {
            client_name = jack_get_client_name(client);
            fprintf (stderr, "unique name `%s' assigned\n", client_name);
        }

        printf ("engine sample rate: %" PRIu32 "\n",
                jack_get_sample_rate (client));

        jack_set_process_callback (client, process, this);
        jack_on_shutdown (client, jack_shutdown, this);

        // create client ports
        input_port[0] = jack_port_register (client, "input_1",
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            JackPortIsInput, 0);
        input_port[1] = jack_port_register (client, "input_2",
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            JackPortIsInput, 0);
        input_port[2] = jack_port_register (client, "input_3",
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            JackPortIsInput, 0);
        input_port[3] = jack_port_register (client, "input_4",
                                            JACK_DEFAULT_AUDIO_TYPE,
                                            JackPortIsInput, 0);
        output_port[0] = jack_port_register (client, "output_1",
                                             JACK_DEFAULT_AUDIO_TYPE,
                                             JackPortIsOutput, 0);
        output_port[1] = jack_port_register (client, "output_2",
                                             JACK_DEFAULT_AUDIO_TYPE,
                                             JackPortIsOutput, 0);

        if ((input_port == nullptr) || (output_port == nullptr)) {
            fprintf(stderr, "no more JACK ports available\n");
            exit (1);
        }


    }

    void start() {

        /* Tell the JACK server that we are ready to roll.  Our
          * process() callback will start running now. */

        if (jack_activate (client)) {
            fprintf (stderr, "cannot activate client");
            exit (1);
        }

        /* Connect the ports.  You can't do this before the client is
         * activated, because we can't make connections to clients
         * that aren't running.  Note the confusing (but necessary)
         * orientation of the driver backend ports: playback ports are
         * "input" to the backend, and capture ports are "output" from
         * it.
         */

        const char **ports;

        //--- connect input
        ports = jack_get_ports (client, nullptr, nullptr,
                                JackPortIsPhysical|JackPortIsOutput);
        if (ports == nullptr) {
            fprintf(stderr, "no ADC ports found\n");
            exit (1);
        }

        if (jack_connect (client, ports[0], jack_port_name (input_port[0]))) {
            fprintf (stderr, "cannot connect ADC port 0\n");
        }

        if (jack_connect (client, ports[1], jack_port_name (input_port[1]))) {
            fprintf (stderr, "cannot connect ADC port 1\n");
        }

        // FIXME: connect additional input ports to supercollider, &c

        free (ports);

        //--- connect output
        ports = jack_get_ports (client, nullptr, nullptr,
                                JackPortIsPhysical|JackPortIsInput);
        if (ports == nullptr) {
            fprintf(stderr, "no DAC ports found\n");
            exit (1);
        }

        if (jack_connect (client, jack_port_name (output_port[0]), ports[0])) {
            fprintf (stderr, "cannot connect DAC port 0\n");
        }
        if (jack_connect (client, jack_port_name (output_port[1]), ports[1])) {
            fprintf (stderr, "cannot connect DAC port 1\n");
        }

        free (ports);

    }

    void stop() {
        jack_deactivate(client);
    }

    void cleanup() {
        jack_client_close(client);
    }
};

JackClient::Imp JackClient::imp;

void JackClient::setup() {
    JackClient::imp.setup();
}

void JackClient::cleanup() {
    JackClient::imp.cleanup();
}

void JackClient::start() {
    JackClient::imp.start();
}

void JackClient::stop() {
    JackClient::imp.stop();
}


const jack_default_audio_sample_t *JackClient::Imp::in_adc[2];
const jack_default_audio_sample_t *JackClient::Imp::in_ext[2];
jack_default_audio_sample_t *JackClient::Imp::out[2];
