import Data.Maybe
import Data.List
import Control.Monad
import Safe
import Text.Printf

main = do
  conts <- getContents
  let ls = filter (\l -> not (null l) && head l /= '#') $ lines conts
      ns = catMaybes $ map run ls

  putStrLn "digraph G {"

  firsts <- forM (zip [(1 :: Int)..] ns) $ \(num, (lbl, deps)) -> do
    forM_ deps $ \d -> do
      printf "  n%d -> n%d;\n" d num
    printf "  n%d [label = \"%s\"];\n" num lbl
    return $ if null deps then Just num else Nothing

  putStr "  { rank = same; "
  forM_ (catMaybes firsts) $ \f -> do
    printf "n%d " f
  putStrLn "; }"

  putStrLn "}"

run :: String -> Maybe (String, [Int])
run str = let tokens = tokenize str
          in if length tokens == 6
                 then let nm = head tokens
                          nums = catMaybes $ map readMay (drop 2 tokens)
                      in if length nums == 4
                           then Just (nm, (filter (/= 0) nums))
                           else Nothing
                 else Nothing

tokenize :: String -> [String]
tokenize = combineQuotes . words

combineQuotes :: [String] -> [String]
combineQuotes = reverse . fst . foldl' fun ([], "")
  where fun (accs, quoted) str | null str  = (accs, quoted)
                               | headQuote && tailQuote = (tail (init str):accs, quoted)
                               | headQuote              = (accs, tail str)
                               | tailQuote              = ((quoted ++ " " ++ init str):accs, "")
                               | null quoted            = (str:accs, quoted)
                               | otherwise              = (accs, quoted ++ " " ++ str)
                    where headQuote = head str == '"'
                          tailQuote = last str == '"'

