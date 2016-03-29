{
  "targets": [{
    "target_name": "action_after_build",
    "type": "none",
    "dependencies": [ "<(module_name)" ],
    "copies": [
      {
        "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
        "destination": "<(module_path)"
      }
    ]
  }, {
    "target_name": "tessocr",
    "sources": [
      "src/tessocr.cc"
    ],
    "include_dirs": [
      "/usr/local/include",
      "<!(node -e 'require(\"nan\")')"
    ],
    "libraries": [
      "-L/usr/local/lib"
    ],
    "link_settings": {
      "libraries": [ "-llept", "-ltesseract" ]
    }
  }]
}
