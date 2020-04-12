minice -- small icecast encoder for stethocast/MP3

Copyright (c) 2001-2006, Masahiko KIMOTO,
                         Ohno Laboratory.

 Programmed by Masahiko KIMOTO (2001-2006)
 All Rights Reserved.


1. miniceについて

miniceはicecastの音源ソースです。以下のような構造をしており、基本的に
は二つのプロセスの標準入出力を経由して得られたバイトストリームを、指定
されたビットレートでicecastに送り出すという機能だけを持っています。

  +---------------------------+
  | ( parent procces )  minice|
  +--------------------+      |       +-------+
     V          V      |      | ====> |icecast|
  +------+  +-------+  |      |       +-------+
  |player|=>|encoder|=>|      |
  +------+  +-------+  +------+

図中の => の箇所はプロセス間通信です。miniceとicecastとの接続はTCP、そ
れ以外の二箇所は名前無しパイプです。miniceはplayerとencoderという二つ
のプロセスをpipeで結んで生成、挙動を監視します。playerの標準出力は
encoderの標準入力に、encoderの標準出力はminiceに接続されます。miniceは
encoderから受け取ったバイトストリームを、設定ファイルで指定されたビッ
トレートでicecastに送り出します。

playerはencoderのどちらかが終了した場合、miniceは残っているプロセスを
終了させた後に再度二つのプロセスを立ち上げます。

miniceは指定された間隔でicecastとの接続を切断し再接続します。この間隔
は秒単位で設定ファイル中で指定できます。指定しない場合は60分になります。
再接続のタイミングは、playerとencoderが指定された時間以上動き続けた場
合か、playerとencoderを再起動する際に前回の再接続から指定時間以上経過
していた場合です。



2. miniceってどういう目的に使えるの

とにかくsimpleな音源ソースで、主な機能は外部プロセスに頼っています。そ
のため、外部プロセスで生成されたオーディオの配信に使えるでしょう。具体
的にはTiMidity(ソフトウェアMIDI音源)とか。



3. miniceのインストールと使い方

3-1. libshoutを用いない場合

インストールはアーカイブを展開したディレクトリに移動して、

 % ./configure ; make

を実行するだけです。miniceという実行形式ができ上がります。

3-2. libshoutを用いる場合

icecastとの接続に、libshoutを用いると、音声ストリームの送出が安定しま
す。通常はこちらを推奨します。libshoutは、FreeBSDならports, NetBSDなら
pkgsrc からインストール可能です。Linuxなどについては、それぞれのディス
トリビューションで配布されている方法を用いてインストールしてください。
configureの際に、libshoutがインストールされているpathを指定します。

 FreeBSDの場合：

   % ./configure --prefix=/usr/local LDFLAGS=-L/usr/local/lib CPPFLAGS=-I/usr/local/include

 NetBSDの場合：

   % ./configure --prefix=/usr/pkg LDFLAGS=-L/usr/pkg/lib CPPFLAGS=-I/usr/pkg/include


3-3. 使い方

miniceは引き数なしで実行できますが、その場合は設定ファイルはカレントディ
レクトリか/etc/にあるものが用いられます。前者が優先されるので注意して
ください。引き数を指定した場合は、その引き数を設定ファイル名として扱い
ます。

重要なのは後述する設定ファイルです。これをきちんと書いた上で、miniceを
実行してください。引数などはありません。設定ファイルがすべてです。もち
ろん、miniceを実行する前に、icecastは立ち上げておく事を忘れずに。


4. 設定ファイル

設定ファイルはminice.confという名前で、configure時に決められるディレク
トリか、カレントディレクトリにあるものを用います。後者が優先されるので
注意してください。

設定ファイルには、一行ごとに変数名と値を空白で区切って記述します。
変数名には以下の項目が指定できます。値は例として示したものです。

server 127.0.0.1

	icecastのホスト名もしくはIPアドレスを指定します。現在のところ
	IPv4のみ対応しています。

port 8000

	icecastのポート番号を指定します。

name icecast

	icecastに渡すcontentsの名前です

genre ice

	icecastに渡すジャンル名です

public 1

	icecastに渡すpublicフラグです。数字です。

url http://www.icecast.org

	icecastに渡すURLです。

mountpoint /
	icecastに渡すmount pointです。

password hackme

	icecastに接続するためのパスワードです

bitrate 64

	icecastに転送するデータのビットレートです。単位はKbpsです。

playlist playlist

	プレイリストを使う場合は、プレイリストのファイルを指定します。
	もしこのファイルが指定された場合は、ファイルの内容を１行ずつ
	読みこんで、playerを実行する度に順番に引き数に渡します。

verbose         1

	冗長フラグです。1以上を指定すると、色々動作メッセージを出します。

authtype x-audio

	認証方法を指定します。icyかx-audioのいずれかを指定します。
	icecast 1.3以降ではx-audioを指定してください。

player mpg123 -s filename.mp3

	playerのコマンドラインを指定します。引き数に%sを書くと、
	その箇所にplaylistから順番にファイル名を挿入します。

encoder lame -x -r -b 64 -f - -

	encoderのコマンドラインを指定します。



3. 利用上の注意

miniceは、あくまで基本機能のみを有します。パラメータなどの調整は自力で
やってもらう必要があります。例えば、minice.confのbitrateで指定するビッ
トレートと、encoderで指定するエンコーダが出力するデータストリームのビッ
トレートは同じでなければなりません。playerの出力とencoderの入力も、各々
の引き数指定などで合わせてもらわないといけません。
