import Enum

{:ok, text} = File.read("../input-2022-1.txt")
chunks = String.split(text, "\n\n")

toint = fn s ->
  case Integer.parse(s) do
    {n, _} -> n
    :error -> 0
  end
end

sum = fn lst ->
  List.foldl(lst, 0, fn acc, v -> acc + v end)
end

sum_nonempty = fn lst ->
  lst |> filter(fn e -> e != '' end)
      |> map(toint)
      |> sum.()
end

sums = map(chunks, fn chunk ->
  chunk |> String.split("\n") |> sum_nonempty.()
end)

sums = sort(sums, fn a, b -> a > b end)
IO.puts(hd(sums))
IO.puts(sums |> slice(0..2) |> sum.())
