#include "BatchControlStage.h"

namespace traffic_manager {

  BatchControlStage::BatchControlStage(
      std::shared_ptr<PlannerToControlMessenger> messenger,
      cc::Client &carla_client)
    : messenger(messenger),
      carla_client(carla_client) {

    // Initializing messenger state.
    messenger_state = messenger->GetState();
    // Initializing number of vehicles to zero in the beginning.
    number_of_vehicles = 0u;
  }

  BatchControlStage::~BatchControlStage() {}

  void BatchControlStage::Action() {

    // Looping over registered actors.
    for (uint i = 0u; i < number_of_vehicles; ++i) {

      cr::VehicleControl vehicle_control;

      PlannerToControlData &element = data_frame->at(i);
      carla::ActorId actor_id = element.actor_id;
      vehicle_control.throttle = element.throttle;
      vehicle_control.brake = element.brake;
      vehicle_control.steer = element.steer;

      commands->at(i) = cr::Command::ApplyVehicleControl(actor_id, vehicle_control);
    }
  }

  void BatchControlStage::DataReceiver() {

    auto packet = messenger->ReceiveData(messenger_state);
    data_frame = packet.data;
    messenger_state = packet.id;

    // Allocating new containers for the changed number of registered vehicles.
    if (data_frame != nullptr &&
        number_of_vehicles != (*data_frame.get()).size()) {

      number_of_vehicles = static_cast<uint>((*data_frame.get()).size());
      // Allocating array for command batching.
      commands = std::make_shared<std::vector<cr::Command>>(number_of_vehicles);
    }

  }

  void BatchControlStage::DataSender() {

    if (commands != nullptr) {
      carla_client.ApplyBatch(*commands.get());
    }

    // Limiting updates to 100 frames per second.
    std::this_thread::sleep_for(10ms);
  }
}