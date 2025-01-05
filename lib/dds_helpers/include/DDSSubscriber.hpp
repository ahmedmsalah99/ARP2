
/**
 * @file HelloWorldSubscriber.hpp
 *
 */
#ifndef SUB_HPP
#define SUB_HPP

#include "TargetsPubSubTypes.hpp"
#include "ObstaclesPubSubTypes.hpp"


#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;


template <typename MyMessage,typename MyMessagePubSubType>
class DDSSubscriber
{
private:
    DomainParticipant *participant_;

    Subscriber *subscriber_;

    DataReader *reader_;

    Topic *topic_;

    TypeSupport type_;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener()
            : samples_(0),matched_(0)
        {
        }

        ~SubListener() override
        {
            delete valid_msg;
        }

        void on_subscription_matched(
            DataReader *reader,
            const SubscriptionMatchedStatus &info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Subscriber matched." << std::endl;
                eprosima::fastdds::rtps::LocatorList locators;
                reader->get_listening_locators(locators);
                for (const eprosima::fastdds::rtps::Locator &locator : locators)
                {
                    print_transport_protocol(locator);
                }
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "Subscriber unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                          << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }
        std::atomic_int matched_;

        void on_data_available(
            DataReader *reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&my_message_, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                if (info.valid_data)
                {
                    samples_++;
                    std::cout << "A new sample received" << std::endl;
                    valid_msg = &my_message_;
                }
            }
        }


        MyMessage* get_valid_data(){
            
            return valid_msg;
        }

    public:
        MyMessage my_message_;
        MyMessage *valid_msg=nullptr;

        std::atomic_int samples_;

    private:
        void print_transport_protocol(const eprosima::fastdds::rtps::Locator &locator)
        {
            switch (locator.kind)
            {
            case LOCATOR_KIND_UDPv4:
                std::cout << "Using UDPv4" << std::endl;
                break;
            case LOCATOR_KIND_UDPv6:
                std::cout << "Using UDPv6" << std::endl;
                break;
            case LOCATOR_KIND_SHM:
                std::cout << "Using Shared Memory" << std::endl;
                break;
            default:
                std::cout << "Unknown Transport" << std::endl;
                break;
            }
        }

    } listener_;

public:
    DDSSubscriber()
        : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new MyMessagePubSubType())
    {
    }

    virtual ~DDSSubscriber()
    {
        
        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
    void waitPub(){
        while(listener_.matched_<1)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //! Initialize the subscriber
    bool init(std::string topic_name)
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");

        //  // Explicit configuration of shm transport
        // participantQos.transport().use_builtin_transports = false;
        // auto shm_transport = std::make_shared<SharedMemTransportDescriptor>();
        // shm_transport->segment_size(10 * 1024 * 1024);
        // participantQos.transport().user_transports.push_back(shm_transport);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the subscriptions Topic
        topic_ = participant_->create_topic(topic_name, type_.get_type_name(), TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr)
        {
            return false;
        }

        // Create the DataReader
        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        if (reader_ == nullptr)
        {
            return false;
        }

        return true;
    }

    

    MyMessage* get_valid_data(){
        return listener_.get_valid_data();
    }

};
#endif // SUB_HPP

// int main()
// {
//     std::cout << "Starting subscriber." << std::endl;
//     uint32_t samples = 10;

//     DDSSubscriber<Targets,TargetsPubSubType>*mysub = new DDSSubscriber<Targets,TargetsPubSubType>();
//     if (mysub->init("Targets"))
//     {
//        while (true)
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         }
//     }

//     delete mysub;
//     return 0;
// }