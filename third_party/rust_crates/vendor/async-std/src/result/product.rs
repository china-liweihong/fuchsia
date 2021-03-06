use std::pin::Pin;

use crate::prelude::*;
use crate::stream::{Product, Stream};

impl<T, U, E> Product<Result<U, E>> for Result<T, E>
where
    T: Product<U>,
{
    #[doc = r#"
        Takes each element in the `Stream`: if it is an `Err`, no further
        elements are taken, and the `Err` is returned. Should no `Err` occur,
        the product of all elements is returned.

        # Examples

        This multiplies every integer in a vector, rejecting the product if a negative element is
        encountered:

        ```
        # fn main() { async_std::task::block_on(async {
        #
        use async_std::prelude::*;
        use async_std::stream;

        let v = stream::from_iter(vec![1, 2, 4]);
        let res: Result<i32, &'static str> = v.map(|x|
            if x < 0 {
                Err("Negative element found")
            } else {
                Ok(x)
            }).product().await;
        assert_eq!(res, Ok(8));
        #
        # }) }
        ```
    "#]
    fn product<'a, S>(stream: S) -> Pin<Box<dyn Future<Output = Result<T, E>> + 'a>>
    where
        S: Stream<Item = Result<U, E>> + 'a,
    {
        Box::pin(async move {
            // Using `take_while` here because it is able to stop the stream early
            // if a failure occurs
            let mut is_error = false;
            let mut found_error = None;
            let out = <T as Product<U>>::product(
                stream
                    .take_while(|elem| {
                        // Stop processing the stream on `Err`
                        !is_error
                            && (elem.is_ok() || {
                                is_error = true;
                                // Capture first `Err`
                                true
                            })
                    })
                    .filter_map(|elem| match elem {
                        Ok(value) => Some(value),
                        Err(err) => {
                            found_error = Some(err);
                            None
                        }
                    }),
            )
            .await;

            if is_error {
                Err(found_error.unwrap())
            } else {
                Ok(out)
            }
        })
    }
}
