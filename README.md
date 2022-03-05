# Carla Trajectory Controller

## Methods

### PID

### Pure Pursuit

### Stanley

### MPC

## Quick Setup

```
./setup_workspace.sh
```

## Run Demo

```
roslaunch trajectory_controller trajectory_control_demo.launch
```

## Clang Format

An easy way to create the .clang-format file is:

```
clang-format -style=llvm -dump-config > .clang-format
```

Available style options are described in [Clang-Format Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html).

## Reference

- [Coursera: Self-Driving Cars Specialization](https://www.coursera.org/learn/motion-planning-self-driving-cars?specialization=self-driving-cars)
