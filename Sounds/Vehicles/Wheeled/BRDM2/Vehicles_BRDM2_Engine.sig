AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 55
   name "EngineLoad"
   tl 384 -256
   children {
    53 56
   }
  }
 }
 Ops {
  IOPItemOpInterpolateClass {
   id 53
   name "Interpolate 53"
   tl 768 -256
   children {
    54
   }
   inputs {
    ConnectionClass connection {
     id 55
     port 0
    }
   }
   "Y min" 0.6
  }
  IOPItemOpFilterClass {
   id 54
   name "Filter 54"
   tl 1024 -256
   children {
    2
   }
   inputs {
    ConnectionClass connection {
     id 53
     port 0
    }
   }
   Frequency 2
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "Exhaust_V"
   tl 1280 -256
   input 54
  }
  IOPItemOutputClass {
   id 56
   name "EngineLoad"
   tl 1280 -64
   input 55
  }
 }
 compiled IOPCompiledClass {
  visited {
   5 134 7 135 6
  }
  ins {
   IOPCompiledIn {
    data {
     2 3 65538
    }
   }
  }
  ops {
   IOPCompiledOp {
    data {
     1 65539 2 0 0
    }
   }
   IOPCompiledOp {
    data {
     1 2 2 1 0
    }
   }
  }
  outs {
   IOPCompiledOut {
    data {
     0
    }
   }
   IOPCompiledOut {
    data {
     0
    }
   }
  }
  processed 5
  version 2
  ops_reeval_list {
   1
  }
 }
}