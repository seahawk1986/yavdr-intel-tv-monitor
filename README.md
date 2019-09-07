# tv-i2c-monitor
This is an experimental program based on [https://github.com/beralt/istvup].
It checks the HDCP chip i2c address availability for the given i2c device
and calls the DBus method of yavdr-frontend to start/stop the currently used frontend.

## Installation
Install the build dependencies:

``` shell
sudo apt install libdbus-1-dev build-essential
```

## Compilation

``` shell
make
```

## Usage

``` shell
# list the available i2c devices
$ sudo i2cdetect -l
i2c-3	i2c       	i915 gmbus dpc                  	I2C adapter
i2c-1	i2c       	i915 gmbus vga                  	I2C adapter
i2c-6	i2c       	DPDDC-A                         	I2C adapter
i2c-4	i2c       	i915 gmbus dpb                  	I2C adapter
i2c-2	i2c       	i915 gmbus panel                	I2C adapter
i2c-0	i2c       	i915 gmbus ssc                  	I2C adapter
i2c-7	i2c       	DPDDC-B                         	I2C adapter
i2c-5	i2c       	i915 gmbus dpd                  	I2C adapter

# with the device number for the "i915 gmbus dp*" connection:
./tv-i2c-monitor 3
```

