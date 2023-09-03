# map dataframes
dfmap <- function(src, fun) {
    ans <- fun(src[1,]);
    for(i in 2:nrow(src)) {
        try(ans <- rbind(ans, fun(src[i,])));
    }

    ans
}

# vec to df
mkdf <- function(arr, fun, resetRowNames=FALSE) {
    ans <- fun(arr[1]);

    for(i in 2:length(arr)) {
        try(ans <- rbind(ans, fun(arr[i])));
    }

    if(resetRowNames) {
        rownames(ans) <- 1:nrow(ans);
    }

    ans
}

# join day quotes into bigger chunks of given amount.
squash <- function(df, chunk) {
    l <- nrow(df);
    n <- ceiling(l/chunk);
    mkdf(0:ceiling(l/n),
         function(i) {
            start <- i*n;
            end <- min(start+n, l);
            subdf <- df[start:end,];
            data.frame(date=subdf$date[1],
                       price=subdf$price[nrow(subdf)],
                       open=subdf$open[1],
                       high=max(subdf$high),
                       low=min(subdf$low))
         });
}

pch_circle <- 21;
pch_up <- 24;
pch_down <- 25;
pch_dot <- 20;

price_chart <- function(df, meanWindow=30) {
    name <- deparse(substitute(df));
    df <- dfmap(df, function(row) {
                        data.frame(date=rep(as.POSIXct(row$date), 2),
                                   price=c(row$open, row$price),
                                   pch=c(ifelse(row$open > row$price, pch_down, pch_up), pch_circle),
                                   extreme=c(row$high, row$low))
                    });
    plot(x=df$date, y=df$price, pch=df$pch, main=name,
         type='b', xlab='date', ylab='price', xaxt="n");
    lxs <- floor(seq(1, nrow(df), nrow(df)/10));
    axis(1, df$date[lxs], format(df$date[lxs], "%Y-%m"));
    points(df$date, df$extreme, pch=pch_dot);
    lines(mkdf(meanWindow:nrow(df), function(i)
                                data.frame(date = df$date[i],
                                           price = mean(df$price[(-meanWindow + 1):1 + i]))),
                             col='#a0a0f0', lwd=3);
}
