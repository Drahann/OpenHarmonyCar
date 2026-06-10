import re

with open('yolov5_nchw_inference_fixed.py', 'r') as f:
    content = f.read()

# Add AIPP-compatible preprocessing function
aipp_func = (
    "\n"
    "def preprocess_image_aipp(image_path, input_shape=(640, 640)):\n"
    "    # AIPP-compatible preprocessing: output NHWC uint8 RGB for OM model with AIPP\n"
    "    img = cv2.imread(image_path)\n"
    "    if img is None:\n"
    "        return None, None, None\n"
    "    \n"
    "    original_img = img.copy()\n"
    "    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)\n"
    "    rgb = cv2.resize(rgb, input_shape, interpolation=cv2.INTER_LINEAR)\n"
    "    # AIPP model expects NHWC uint8 format\n"
    "    img_batch = np.expand_dims(rgb, axis=0)  # [1, H, W, 3] uint8\n"
    "    ori_shape = np.array([original_img.shape[1], original_img.shape[0]], np.int32)\n"
    "    \n"
    "    return img_batch, original_img, ori_shape\n"
    "\n\n"
)

# Insert the new function before preprocess_image_correct
content = content.replace(
    'def preprocess_image_correct(',
    aipp_func + 'def preprocess_image_correct('
)

# Update inference_single_image to use AIPP preprocessing
content = content.replace(
    'img_batch, original_img, ori_shape = preprocess_image_correct(image_path, (640, 640))',
    'img_batch, original_img, ori_shape = preprocess_image_aipp(image_path, (640, 640))'
)

with open('yolov5_nchw_inference_fixed.py', 'w') as f:
    f.write(content)

print('Fix applied successfully')
