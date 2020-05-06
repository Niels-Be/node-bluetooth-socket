{
  'targets': [
    {
      'target_name': 'BluetoothFd',
      'sources': [
        'src/bluetooth-fd.cc'
      ],
      "include_dirs" : [
            "<!(node -e \"require('nan')\")"
        ]
    }
  ]
}