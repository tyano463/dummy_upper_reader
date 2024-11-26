# SUMMARY

上リーダのQemu用ダミーです。

別途以下を必要とします。

- [仮想ttyデバイスドライバ](https://github.com/tyano463/cdev_ipc)
- [RA2L1のUARTをローカルに閉じたもの](https://github.com/tyano463/qemu.git)

Qemuはブランチを`renesas/ra2l1`にする必要があります。

# 処理の流れ

## UART

![kroki.io](https://kroki.io/graphviz/svg/eNp1kEtLw0AQx-_5FMue1XoPETx4Eg-KOQUJSXfahOblblIsJSDJpR48-TgJehBBei_4-jQLLf0W7m4ehkrnMOzM_nZm_3_iD6mTeMg8PDtHUw2JiGICFksnARiYxllEgOAdxDwnEQ03vsIXCgMyBIv4VPZSr26qxDK3GtoPMpYCNevBMgLHhQAZCPPyjZffvFiIvHr_XD_e4RZqD37EEuinlnpl4OXr0_L5ixc_vHzg5azeKeMSwqyl7j_W1y-rm9np0YnZYUgWhpMGUoWdJQlQm4JDgHbIzf1o90Bt-A9MqRONhBzmhKArRq_26PmfH_l2a443rTEwL-bSmPKWl_OtxqRpq6RHYNwTtVS735VL_THQhmJ-mARgD_xBvDeKBdYdJfVVeP1frfFU3ghA64JSnpZrv1zNtA0=)

<summary>
<details>graphvizのコード</details>

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

</summary>
