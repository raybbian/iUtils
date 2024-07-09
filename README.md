# iUtils

This is a collection of tools to chip away at Apple's walled garden, exposing and utilizing hidden USB interfaces to provide user functionality.

## Drivers

### Keystone Driver

As its name suggests, this driver is the keystone of the project. Its roles and responsibilities are as follows:

1. _[COMPLETE]_ **Communicate with the USB stack below**: Send internal device control IRPs to the USB host controller. This is done to get device descriptors, configuration descriptors, set the device configuration, as well as send vendor specific URB's enable the device's hidden modes.
2. _[IN PROGRESS]_ **User configuration and request handling**: Handle IOCTL requests to the FDO from the user mode app that allow the user to select configurations, device modes, and control other aspects of the device - these messages are validated and sent to the USB stack below.
3. _[IN PROGRESS]_ **Core driver functionality**: Handle the enumeration of the device's interfaces (grouped by function), and create a PDO (physical device object) for each of them. Each of these PDO's are initialized with information that allow Microsoft to choose drivers for this interface.
4. _[COMPLETE]_ **Communicate with the USB stack above**: Handle IOCTL requests to the child PDO's from drivers above, including responding to URB's that request for device descriptors, such that the drivers for these functions operate normally.

### Valeria Driver

This driver will be implemented to allow users to see their iDevice screen in the form of webcam input. Potentially see [quicktime_video_hack](https://github.com/danielpaulus/quicktime_video_hack).

## Helper Apps

### Configurator

This is the user application that will allow the user to:

1. Install the driver to the device (using [libwdi](https://github.com/pbatard/libwdi)).
2. Configure the iDevice's USB configuration.
3. Enable or disable specific functions (PTP/iTunes/Valeria/CDC NCM) depending on the configurations.
4. View stats and information about the device such as transfer rate, charging speed, etc.

### Messenger

This is a DLL that will allow you to access the API for the Keystone driver through code, to integrate into your application.

## What iUtilities Offers

These are some applications that become possible, based on this driver's functionality:

- **Monitor Mirroring/Extending to iDevice**: Using a USB-C cable and the hidden **Network** device mode, one can mirror/extend their screen to their iDevice using applications such as [Moonlight](https://moonlight-stream.org/) over "Ethernet" with virtually zero latency. One could experiment with sending raw video at lower resolutions, turning their iDevice into a true "extra monitor".

- **Virtual Apple Capture Card**: Using the hidden **Valeria** device mode, one can record and share their iDevice's screen using applications such as [OBS](https://obsproject.com/) directly onto their PC without the need to store video on the iDevice.

- **Super Speed File Transfers**: Using the hidden **Network** device mode, one can set up a P2P connection and transfer files at speeds up to 3Gbps (375 Megabytes per second, much faster that 99% of home networks)!

## What iUtilities Preserves

Because iUtilities simply extends the capabilities of your iDevice, the following functionalities can still be used without issue:

- **Itunes Backup and Communication**: The USBMUX USB interface that Apple uses for iTunes and device communication is simply passed through to Apple's own driver.

- **PTP Photo Transfer and Sharing**: The PTP USB interface is similary passed through to the Windows MTP driver - you can still share, access, and backup photos on your iDevice through your Windows machine.

- **Mobile Tethering**: You will still be able to share your mobile data over USB to your computer.

- **iTunes Audio Streaming**: Not too sure what this feature does. Allows the user to play music from iTunes?

- **Charging Device**: A user's iDevice will still be able to charge normally.
