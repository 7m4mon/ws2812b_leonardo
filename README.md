# ws2812b_leonardo
ws2812b_leonardo

    <img alt="" src="ws2812b-controller-arduino-2.jpg"><br>
    <img alt="" src="ws2812b-controller-arduino-1.jpg"><br>
    
    <p>SDカードに保存されたBMPファイルをLEDマトリクスに表示します。<br>
      SDカードの最大容量は32GBです。</p>
<p>電源ONでSDカードにアクセスし、ルートフォルダのファイルに順にアクセスします。<br>
      ファイルの属性をチェックし、ヘッダが<span style="font-weight: bold;">32bit</span>のBMPファイルだったならば、<br>
      幅と高さを確認して、OKだったらファイルに1バイトずつアクセスして、<br>
      R,G,Bを順に読み込み、明るさを調整して、LEDマトリクスの配列に格納します。<br>
      <br>
      アニメーションを実現するには、LEDマトリクスの大きさ毎に画像を配列した<br>
      BMPファイルを作成します。</p>
      <img alt="" src="https://raw.githubusercontent.com/7m4mon/ws2812b_leonardo/master/mario-anim_a_p200.bmp">
      <br>
      BMPファイルのヘッダのReserve領域①(bfReserved1)に『LEDマトリクスの幅－１』<br>
      （LEDマトリクスの幅が16ならば15 = 0x0F）を入力します。<br>
      ビットマップファイルのヘッダはリトルエンディアンなので、0x0F,0x00です。</p>
    <p>アニメーション表示、スクロールスピードはReserve領域②(bfReserved2)に値を入れて制御します。<br>
      その他の値と同様、リトルエンディアンで入力します。<br>
      追加のポーズ時間が200msなら、0xC8, 0x00 です。</p>
    <img alt="" src="bmp_header_anim.PNG">
    <p><br>
      予約領域に入れた値は画像編集ソフトで保存すると0に上書きされてしまうので<br>
      要注意です。また、Windowsの『ペイント』は24bit Bitmapで保存するので<br>
      他のソフトで32bit（ARGB）で保存し直す必要があります。<br>
      <br>
