# Carla Trajectory Controller

## Methods

The longitudinal control is implemented with a PID Controller, while the later controller is optional to use one of the three: Pure Pursuit, Stanley and MPC.

## Quick Setup

```
./setup_workspace.sh
```

## Run Demo

**Pure Pursuit**

```
roslaunch trajectory_controller trajectory_control_demo.launch control_method:="PurePursuit"
```

**Stanley**

```
roslaunch trajectory_controller trajectory_control_demo.launch control_method:="Stanley"
```

**Pure Pursuit**

```
roslaunch trajectory_controller trajectory_control_demo.launch control_method:="MPC"
```

## Clang Format

An easy way to create the .clang-format file is:

```
clang-format -style=llvm -dump-config > .clang-format
```

Available style options are described in [Clang-Format Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html).

## Reference

- [Coursera: Self-Driving Cars Specialization](https://www.coursera.org/learn/motion-planning-self-driving-cars?specialization=self-driving-cars)
- [Lane-Keeping-Assist-on-CARLA(Python)](https://github.com/paulyehtw/Lane-Keeping-Assist-on-CARLA)
- [Three Methods of Vehicle Lateral Control: Pure Pursuit, Stanley and MPC](https://dingyan89.medium.com/three-methods-of-vehicle-lateral-control-pure-pursuit-stanley-and-mpc-db8cc1d32081)
