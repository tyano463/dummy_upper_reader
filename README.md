# SUMMARY

上リーダのQemu用ダミーです。

別途以下を必要とします。

- [仮想ttyデバイスドライバ](https://github.com/tyano463/cdev_ipc)
- [RA2L1のUARTをローカルに閉じたもの](https://github.com/tyano463/qemu.git)

Qemuはブランチを`renesas/ra2l1`にする必要があります。

# 処理の流れ

## UART

```dot
digraph UART {
    node[style="rounded", shape="box"]
    edge[dir="both"]
    
    subgraph clusterU {
        label = "ユーザー空間"
        
        inspect[label="検査ソフト"]
        qemu[label="改造版QEMU"]
        dummy[label="dummy_upper_reader"]
        
        inspect -> qemu
        
        {rank = same; qemu; dummy;}
    
    }
    
    subgraph clusterK {
        label="カーネル空間"
        
        tty[label="/dev/ttyQEMU0"]
        driver[label="simple_fifo.ko"]

        tty -> driver
    }

    qemu -> tty
    tty -> dummy
}
```

