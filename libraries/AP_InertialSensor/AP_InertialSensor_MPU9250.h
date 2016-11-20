#pragma once

#include <stdint.h>

#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/I2CDevice.h>
#include <AP_HAL/SPIDevice.h>
#include <AP_Math/AP_Math.h>
#include <Filter/Filter.h>
#include <Filter/LowPassFilter2p.h>

#include "AP_InertialSensor_Backend.h"
#include "AP_InertialSensor.h"
#include "AuxiliaryBus.h"

class AP_MPU9250_AuxiliaryBus;
class AP_MPU9250_AuxiliaryBusSlave;

// enable debug to see a register dump on startup
#define MPU9250_DEBUG 0

class AP_InertialSensor_MPU9250 : public AP_InertialSensor_Backend
{
    friend AP_MPU9250_AuxiliaryBus;
    friend AP_MPU9250_AuxiliaryBusSlave;

public:
    virtual ~AP_InertialSensor_MPU9250();

    static AP_InertialSensor_MPU9250 &from(AP_InertialSensor_Backend &backend) {
        return static_cast<AP_InertialSensor_MPU9250&>(backend);
    }

    static AP_InertialSensor_Backend *probe(AP_InertialSensor &imu,
                                            AP_HAL::OwnPtr<AP_HAL::I2CDevice> dev,
                                            enum Rotation rotation = ROTATION_NONE);

    static AP_InertialSensor_Backend *probe(AP_InertialSensor &imu,
                                            AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev,
                                            enum Rotation rotation = ROTATION_NONE);

    /* update accel and gyro state */
    bool update();

    /*
     * Return an AuxiliaryBus if the bus driver allows it
     */
    AuxiliaryBus *get_auxiliary_bus() override;

    void start() override;

private:
    AP_InertialSensor_MPU9250(AP_InertialSensor &imu,
                              AP_HAL::OwnPtr<AP_HAL::Device> dev,
                              enum Rotation rotation);

#if MPU9250_DEBUG
    static void _dump_registers();
#endif

    bool _init();
    bool _hardware_init();

    void _set_filter_register(uint16_t filter_hz);
    bool _has_auxiliary_bus();

    /* Read a single sample */
    bool _read_sample();

    void _fifo_reset();
    void _fifo_enable();
    
    /* Check if there's data available by reading register */
    bool _data_ready();
    bool _data_ready(uint8_t int_status);

    /* Read and write functions taking the differences between buses into
     * account */
    bool _block_read(uint8_t reg, uint8_t *buf, uint32_t size);
    uint8_t _register_read(uint8_t reg);
    void _register_write(uint8_t reg, uint8_t val, bool checked=false);

    void _accumulate(uint8_t *samples, uint8_t n_samples);
    void _accumulate_fast_sampling(uint8_t *samples, uint8_t n_samples);
    void _check_temperature(void);

    // instance numbers of accel and gyro data
    uint8_t _gyro_instance;
    uint8_t _accel_instance;

    float _temp_filtered;
    LowPassFilter2pFloat _temp_filter;
    
    AP_HAL::OwnPtr<AP_HAL::Device> _dev;
    AP_MPU9250_AuxiliaryBus *_auxiliary_bus;

    enum Rotation _rotation;

    // are we doing more than 1kHz sampling?
    bool _fast_sampling;

    // has master i2c been enabled?
    bool _master_i2c_enable;
    
    // last temperature reading, used to detect FIFO errors
    float _last_temp;
    uint8_t _temp_counter;

    // buffer for fifo read
    uint8_t *_fifo_buffer;

    uint8_t _reg_check_counter;

    // accumulators for fast sampling
    struct {
        Vector3l accel;
        Vector3l gyro;
        uint8_t count;
    } _accum;
};

class AP_MPU9250_AuxiliaryBusSlave : public AuxiliaryBusSlave
{
    friend class AP_MPU9250_AuxiliaryBus;

public:
    int passthrough_read(uint8_t reg, uint8_t *buf, uint8_t size) override;
    int passthrough_write(uint8_t reg, uint8_t val) override;

    int read(uint8_t *buf) override;

protected:
    AP_MPU9250_AuxiliaryBusSlave(AuxiliaryBus &bus, uint8_t addr, uint8_t instance);
    int _set_passthrough(uint8_t reg, uint8_t size, uint8_t *out = nullptr);

private:
    const uint8_t _mpu9250_addr;
    const uint8_t _mpu9250_reg;
    const uint8_t _mpu9250_ctrl;
    const uint8_t _mpu9250_do;

    uint8_t _ext_sens_data = 0;
};

class AP_MPU9250_AuxiliaryBus : public AuxiliaryBus
{
    friend class AP_InertialSensor_MPU9250;

public:
    AP_HAL::Semaphore *get_semaphore() override;

protected:
    AP_MPU9250_AuxiliaryBus(AP_InertialSensor_MPU9250 &backend, uint32_t devid);

    AuxiliaryBusSlave *_instantiate_slave(uint8_t addr, uint8_t instance);
    int _configure_periodic_read(AuxiliaryBusSlave *slave, uint8_t reg,
                                 uint8_t size);

private:
    void _configure_slaves();

    static const uint8_t MAX_EXT_SENS_DATA = 24;
    uint8_t _ext_sens_data = 0;
};
