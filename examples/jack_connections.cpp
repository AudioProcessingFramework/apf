// Example showing how to connect JACK ports.

#include "apf/mimoprocessor.h"
#include "apf/jack_policy.h"
#include "apf/cxx_thread_policy.h"

class MyProcessor : public apf::MimoProcessor<MyProcessor
      , apf::jack_policy, apf::cxx_thread_policy>
{
  public:
    MyProcessor()
    {
      Input::Params in_params;

      in_params.set("port_name", "no_initial_connection");
      this->add(in_params);

      in_params.set("port_name", "initial_connection");
      in_params.set("connect_to", "system:capture_1");
      this->add(in_params);

      Output::Params out_params;

      out_params.set("port_name", "connect_before_activate");
      this->add(out_params);

      out_params.set("port_name", "connect_after_activate");
      this->add(out_params);

      out_params.set("port_name", "port with spaces");
      this->add(out_params);
    }
};

int main()
{
  MyProcessor processor;

  processor.connect_ports("MimoProcessor:connect_before_activate"
      , "system:playback_1");

  sleep(5);

  processor.activate();

  sleep(2);

  processor.connect_ports("MimoProcessor:connect_after_activate"
      , "system:playback_1");

  processor.connect_ports("MimoProcessor:port with spaces"
      , "system:playback_2");

  sleep(30);
  processor.deactivate();
}

// Settings for Vim (http://www.vim.org/), please do not remove:
// vim:softtabstop=2:shiftwidth=2:expandtab:textwidth=80:cindent
