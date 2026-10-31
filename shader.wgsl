// Declaration of the inputBuffer resource,
// attached to binding #0 of bind group #0
@group(0) @binding(0)
var<storage,read> inputBuffer: array<f32>;

// Declaration of the outputBuffer resource,
// attached to binding #1 of bind group #0
@group(0) @binding(1)
var<storage,read_write> outputBuffer: array<f32>;

@compute @workgroup_size(32)
fn computeStuff(@builtin(global_invocation_id) threadId: vec3u) {
  // Core behavior of our shader: multiply input by 2 and store result
  // in output buffer:
  outputBuffer[threadId.x] = 2.0 * inputBuffer[threadId.x];
}
