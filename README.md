# GenMap

## TODO

[x] Make loading an asset more generic so a sprite or studio model can also be the asset loaded from arguments
[x] Support sprites
[ ] Shade the models from their environment (find the closest face and extract an average color from its lightmap)
[ ] Make the code base compile on linux (see older opengl projects for example code)
[ ] Render all bsp models, not only the worldspawn
[ ] Make sure all render modes are working
[ ] Add imgui windows for render settings

## Render modes

The following render modes are supported by the GoldSrc engine.

### NormalBlending = 0

![Example of normal blending in GoldSrc](NormalBlending.png)

The ``FX Amount`` is set to 128 on the cycler_sprite and env_sprite, and 255 on the brush.

### ColorBlending = 1

![Example of color blending in GoldSrc](ColorBlending.png)

The ``FX Amount`` is set to 128 on the cycler_sprite and env_sprite, and 255 on the brush.

### TextureBlending = 2

![Example of texture blending in GoldSrc](TextureBlending.png)

The ``FX Amount`` is set to 128 on the cycler_sprite and env_sprite, and 255 on the brush.

### GlowBlending = 3

![Example of glow blending in GoldSrc](GlowBlending.png)

The ``FX Amount`` is set to 128 on the cycler_sprite and env_sprite, and 255 on the brush.

Only sprite can be set to glow, an MDL of a gemetry model will trigger an erro in the console.

### SolidBlending = 4

![Example of solid blending in GoldSrc](SolidBlending.png)

The ``FX Amount`` is set to 255 on all the entities.

### AdditiveBlending = 5

![Example of additive blending in GoldSrc](AdditiveBlending.png)

The ``FX Amount`` is set to 128 on the cycler_sprite and env_sprite, and 255 on the brush.
