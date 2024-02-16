#include <IRBlaster.hpp>

/**
 * @brief Destroy the IRBlaster object 
*/
IRBlaster::~IRBlaster()
{
    ESP_ERROR_CHECK(rmt_driver_uninstall(channel));
}

/**
 * @brief Send an IR signal
 * 
 * @param data The microseconds timing array produced by IRReceiver
 * @param size The number of elements in the array
*/
void IRBlaster::send(const uint16_t *data, const size_t size)
{
    // Send a raw IR signal
    rmt_item32_t *items = new rmt_item32_t[size / 2 + size % 2];
    // data is in microseconds, we need to convert it to ticks
    // If the number of elements is odd, we need to add a 0 at the end
    for (size_t i = 0, j = 0; i < size; i += 2, j++)
    {
        items[j].level0 = 1;
        items[j].duration0 = data[i];
        items[j].level1 = 0;
        if (i + 1 < size)
        {
            items[j].duration1 = data[i+1];
        } else {
            items[j].duration1 = 0;
        }
    }
    ESP_ERROR_CHECK(rmt_write_items(channel, items, size / 2 + size % 2, false));
    delete[] items;
}

/**
 * @brief Construct a new IRBlaster object
 * 
 * @param pin The pin to use for IR transmission
 * @param channel The RMT channel to use
*/

IRBlaster::IRBlaster(const uint8_t pin, rmt_channel_t channel)
{
    this->channel = channel;
    gpio_num_t gpio = gpio_num_t(pin);
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio, channel);
    config.clk_div = 80;
    config.tx_config.carrier_en = true;
    config.tx_config.carrier_freq_hz = 38000;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(channel, 0, 0));
}

/**
 * @brief Construct a new IRBlaster object
 * @note This constructor uses RMT_CHANNEL_0
 * 
 * @param pin The pin to use for IR transmission
*/
IRBlaster::IRBlaster(const uint8_t pin)
{
    IRBlaster(pin, RMT_CHANNEL_0);
}