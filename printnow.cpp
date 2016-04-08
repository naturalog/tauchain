	bu = "http://idni.org/builtins#printNow";
	bui = dict.set(mkiri(pstr(bu)));
	builtins[bui].push_back(
			[bu, entry](Thing *s_, Thing *o_) mutable {
				switch (entry) {
					case 0: {
						auto s = getValue(s_);
				
						auto o = getValue(o_);
				
						dout << str(s_) << " printNow " << str(o_) << endl;
						entry = LAST;
						return true;
					}
					case_LAST:;
						END;
				}
			}
	);
