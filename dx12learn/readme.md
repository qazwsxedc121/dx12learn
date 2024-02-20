# Graphic framework using DirectX12

## features

- json config everything
- texture mapping
- alpha blend
- fast json parsing using simdjson
- simple animation

## snapshot

![snap](/Doc/snap1.png, "snap")

## how to use

change view by mouse

Currently every thing rendered is controlled by ``Assets/Data/scene.json``

this is another simpler example:

```
{
    "mesh" : [
        {
            "name": "box",
            "type": "box",
            "param": {
                "width": 1.0,
                "height": 1.5,
                "depth": 1.5,
                "num_subdivision": 3
            },
            "material": "rock"
        },
        {
            "name": "sphere",
            "type": "sphere",
            "param": {
                "radius": 0.5,
                "slice": 20,
                "stack": 20
            },
            "material": "wood"
        },
        {
            "name": "cylinder",
            "type": "cylinder",
            "param": {
                "bottom_radius": 0.5,
                "top_radius": 0.3,
                "height": 3.0,
                "slice": 20,
                "stack": 20
            },
            "material": "wood"
        },
        {
            "name": "grid",
            "type": "grid",
            "param": {
                "width": 20.0,
                "depth": 30.0,
                "m": 60,
                "n": 40
            },
            "material": "rock"
        }
    ],
    "mesh_instance" : [
        {
            "name": "box", 
            "world": [0.0, 0.5, 0.0]
        },
        {
            "name": "grid"
        },
        {
            "name": "cylinder",
            "world": [-5.0, 1.5, -10.0]
        }
    ],
    "material" : [
        {
            "name": "grass",
            "diffuse_tex": 0,
            "fresnel_r0": [0.01, 0.01, 0.01],
            "roughness": 0.5,
            "mat_transform": [100.0, 0.0, 0.0, 0.0,
                              0.0, 100.0, 0.0, 0.0,
                              0.0, 0.0, 100.0, 0.0,
                              0.0, 0.0, 0.0, 1.0]
        },
        {
            "name": "rock",
            "diffuse_tex": 1,
            "fresnel_r0": [0.01, 0.01, 0.01],
            "roughness": 0.5
        },
        {
            "name": "wood",
            "diffuse_tex": 2,
            "fresnel_r0": [0.01, 0.01, 0.01],
            "roughness": 0.5
        },
        {
            "name": "water",
            "diffuse_tex": 3,
            "diffuse_albedo": [1.0, 1.0, 1.0, 0.5],
            "fresnel_r0": [0.02, 0.02, 0.02],
            "roughness": 0.1
        }
    ],
    "texture": [
        {
            "name":"forrest_ground", 
            "path":"./Textures/forrest_ground_01_1k/forrest_ground_01_diff_1k.dds"
        },
        {
            "name": "river_pebble", 
            "path": "./Textures/ganges_river_pebbles_1k/ganges_river_pebbles_diff_1k.dds"
        },
        {
            "name": "raw_plank_wall", 
            "path": "./Textures/raw_plank_wall_1k/raw_plank_wall_diff_1k.dds"
        },
        {
            "name": "water",
            "path": "./Textures/water/water1.dds"
        }
    ]
}
```
