----------------------------------------------------------------------
--  $Id: exe2str.hs,v 1.1 2005/05/19 21:22:10 btittelbach Exp $
----------------------------------------------------------------------

import IO
import System
import Char
import List

swap :: (a, b) -> (b, a)
swap (x, y) = (y, x)

octal 0 = Nothing
octal x = Just $ swap $ divMod x 8;

chr2octesc :: Char -> [Char]
chr2octesc '\0' = "\\0"
chr2octesc c = "\\" ++ (map (chr.(\x->x+(ord '0'))) $ reverse $ unfoldr octal $ ord c)

printfile filename = do
	putStrLn $ "\n" ++ filename ++ ":"
	readFile (filename) >>= \contents -> putStrLn $ "\"" ++ (concatMap chr2octesc contents) ++ "\""

main = do
	args <- getArgs
	mapM printfile args

