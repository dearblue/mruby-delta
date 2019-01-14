# mruby-delta

文字列オブジェクトをバイト配列として見立て、隣り合うバイト値ごとの差分値を計算します。

例えば PNG 画像の `PNG_FILTER_SUB` フィルタ、あるいは lzma/xz の `delta` フィルタ。


## できること

  - `Delta.encode(string) -> new string`
  - `Delta.encode(string, [dest,] type: Delta::INT8, stripe: 1) -> new string or dest`
  - `Delta.encode(string, [dest,] type: Delta::INT8, stripe: 1, round: 1) -> new string or dest`
  - `Delta.encode(string, [dest,] type: Delta::INT8, stripe: 1, prediction: true) -> new string or dest`
  - `Delta.encode(string, [dest,] type: Delta::INT8, stripe: 1, weight: [1, 1], gain: nil) -> new string or dest`

- - - -

  - `Delta.decode(string) -> new string`
  - `Delta.decode(string, [dest,] type: Delta::INT8, stripe: 1) -> new string or dest`
  - `Delta.decode(string, [dest,] type: Delta::INT8, stripe: 1, round: 1) -> new string or dest`
  - `Delta.decode(string, [dest,] type: Delta::INT8, stripe: 1, prediction: true) -> new string or dest`
  - `Delta.decode(string, [dest,] type: Delta::INT8, stripe: 1, weight: [1, 1], gain: nil) -> new string or dest`

- - - -

  - `Delta.entoropy(string) -> float`


## くみこみかた

`build_config.rb` に gem として追加して、mruby をビルドして下さい。

```ruby
MRuby::Build.new do |conf|
  conf.gem "mruby-delta", github: "dearblue/mruby-delta"
end
```

- - - -

mruby gem パッケージとして依存したい場合、`mrbgem.rake` に記述して下さい。

```ruby
# mrbgem.rake
MRuby::Gem::Specification.new("mruby-XXX") do |spec|
  ...
  spec.add_dependency "mruby-delta", github: "dearblue/mruby-delta"
end
```


## つかいかた

TODO: そのうちかく


## せってい

  - `DELTA_FORCE_INLINE_EXTRACT` - `stripe=(1..2)` および `round=(1..4)` の範囲の関数を強制的にインライン展開させる指示を与えます。
    速度の向上が見込めますが、オブジェクトコードが肥大化します。

    ```ruby
    # A part of build_config.rb

    conf.cc.defines << "DELTA_FORCE_INLINE_EXTRACT"
    ```

  - `DELTA_WEAK_INLINE` - 強制インラインとするための修飾子を直接指定します。
    `DELTA_FORCE_INLINE_EXTRACT` がうまく動作しない場合に試すべきです。

    ```ruby
    # A part of build_config.rb

    conf.cc.defines << "DELTA_WEAK_INLINE=__attribute__((always_inline))"
    ```


## Specification

  - Package name: mruby-delta
  - Version: 0.1
  - Product quality: PROTOTYPE, EXPERIMENTAL, UNSTABLE, ***サラマンダーよりずっと遅い！***
  - Author: [dearblue](https://github.com/dearblue)
  - Project page: <https://github.com/dearblue/mruby-delta>
  - Licensing: [2 clause BSD License](LICENSE)
  - Object code size: about 100 KiB (with gcc `-O3` option) / about 40 KiB (with gcc `-Os` option)
  - Bundled C libraries (git-submodules): (NONE)
  - Dependency external mrbgems:
      - [mruby-aux](https://github.com/dearblue/mruby-aux)
        under [Creative Commons Zero License \(CC0\)](https://github.com/dearblue/mruby-aux/blob/master/LICENSE)
        by [dearblue](https://github.com/dearblue)
