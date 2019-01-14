#!ruby

# 1, 2, 3, 4, 6, 8 バイトで割り切れるもの (最小は24バイト)
s = ("0123456789abcdefghijklmnopqrstuvwxyz" * 20).freeze

[Delta::INT8, Delta::INT16BE, Delta::INT24BE,
 Delta::INT32BE, Delta::INT48BE, Delta::INT64BE,
 :"int16le", :"Int24le", "int32LE", "inT48lE", "INT64LE"].each do |type|
  [1, 2, 4].each do |stripe|
    [1, 2, 3, 4, 5].each do |round|
      assert "(type=#{type}, stripe=#{stripe}, round=#{round})" do
        ss = Delta.encode(s, type: type, stripe: stripe, round: round)
        assert_not_equal s, ss
        sss = Delta.decode(s, type: type, stripe: stripe, round: round)
        assert_not_equal s, sss
      end
    end

    [false, true].each do |prediction|
      assert "(type=#{type}, stripe=#{stripe}, prediction=#{prediction})" do
        ss = Delta.encode(s, type: type, stripe: stripe, prediction: prediction)
        assert_not_equal s, ss
        sss = Delta.decode(s, type: type, stripe: stripe, prediction: prediction)
        assert_not_equal s, sss
      end
    end

    [
      # これらの数字に根拠なし
      { weight: [2, 5] },
      { weight: [2, 5], gain: 1000 },
      { weight: [2, 5], gain: -1000 },
      { weight: [8, 1, 7, 2, 6, 3, 5, 4] },
      { weight: [8, 1, 7, 2, 6, 3, 5, 4], gain: 1000 },
      { weight: [8, 1, 7, 2, 6, 3, 5, 4], gain: -1000 },
    ].each do |opts|
      assert "(type=#{type}, stripe=#{stripe}, opts=#{opts.inspect})" do
        ss = Delta.encode(s, type: type, stripe: stripe, **opts)
        assert_not_equal s, ss
        sss = Delta.decode(s, type: type, stripe: stripe, **opts)
        assert_not_equal s, sss
      end
    end
  end
end

# 間違った引数のテストも行う
